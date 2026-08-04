#include "Utilities.h"
#include "WorldspaceCatalog.h"

#include <shared_mutex>

namespace WMS::WorldspaceCatalog
{
    namespace
    {
        // entries is shared by the game-start builder and later UI/command readers.
        // shared_mutex permits multiple readers but only one writer.
        std::vector<MapOption> entries;
        std::shared_mutex entriesLock;

        // Convert Skyrim's borrowed C string into an owned std::string.
        std::string SafeText(const char* text, std::string_view fallback)
        {
            return text && text[0] ? text : std::string(fallback);
        }

        bool IsMapCandidate(RE::TESWorldSpace* worldspace)
        {
			// Only list worldspaces that own their own map data.
            if (!worldspace || GetMapOwner(worldspace) != worldspace) return false;

            // A usable world map needs non-degenerate cell bounds.
            // This also admits map data supplied to normally mapless worlds by mods.
            const auto& map = worldspace->worldMapData;
            return map.nwCellX != map.seCellX || map.nwCellY != map.seCellY;
        }

        // Mark every option that shares its displayed name with another map.
        // This runs once while building the catalog rather than for every chooser button.
        void MarkDuplicateDisplayNames(std::vector<MapOption>& options)
        {
            for (std::size_t left = 0; left < options.size(); ++left) {
                for (std::size_t right = left + 1; right < options.size(); ++right) {
                    if (Utilities::EqualsIgnoreCase(options[left].displayName, options[right].displayName)) {
                        options[left].hasDuplicateDisplayName  = true;
                        options[right].hasDuplicateDisplayName = true;
                    }
                }
            }
        }

        // Sort alphabetically by display name, using EditorID to make equal names deterministic.
        bool CompareMapOptions(const MapOption& left, const MapOption& right)
        {
			// stricmp is a Microsoft extension that compares two C strings case-insensitively.
			// It returns a negative value if left < right, zero if equal, and a positive value if left > right.
            const auto nameComparison = _stricmp(left.displayName.c_str(), right.displayName.c_str());
            if (nameComparison != 0) {
                return nameComparison < 0;
            } else {
				const auto editorIDComparison = _stricmp(left.editorID.c_str(), right.editorID.c_str());
                return editorIDComparison < 0;
            }
        }

        // Move one worldspace to a preferred position while preserving the order of the other options.
        bool MoveToIndex(std::vector<MapOption>& options, RE::TESWorldSpace* worldspace, std::size_t index)
        {
            if (!worldspace || index >= options.size()) return false;

            const auto destination = std::next(options.begin(), index);

            // The projection &MapOption::worldspace tells ranges::find to compare worldspace against that member of each MapOption.
            const auto option = std::ranges::find(destination, options.end(), worldspace, &MapOption::worldspace);
            if (option == options.end()) return false;

			// std::rotate moves the element at option to destination, shifting the intervening elements to the right.
            std::rotate(destination, option, std::next(option));
            return true;
        }

    }

    // Called after SKSE's DataLoaded message, when forms from every active plugin are available and their runtime FormIDs are final.
    void Build()
    {
        auto* dataHandler = RE::TESDataHandler::GetSingleton();
        if (!dataHandler) {
            SKSE::log::error("Could not get the data handler for the map catalog.");
            return;
        }

        std::vector<MapOption> newEntries;

        // Range-based for visits every loaded TESWorldSpace form.
        for (auto* worldspace : dataHandler->GetFormArray<RE::TESWorldSpace>()) {
            if (!IsMapCandidate(worldspace)) continue;

            const auto* file = worldspace->GetFile(0);
            if (!file) {
                SKSE::log::warn("Ignoring dynamic map candidate {:08X}; it has no originating plugin.", worldspace->GetFormID());
                continue;
            }

            // Designated initializers name each MapOption field being filled.
            newEntries.push_back({
                .worldspace  = worldspace,
                .displayName = SafeText(worldspace->GetName(), "<unnamed>"),
                .editorID    = SafeText(worldspace->GetFormEditorID(), ""),
                .pluginName  = std::string(file->GetFilename())
            });
        }

        MarkDuplicateDisplayNames(newEntries);

        {
            // unique_lock excludes readers while replacing the shared catalogue.
            std::unique_lock lock(entriesLock);
            // move transfers the vector's allocation instead of copying every entry.
            entries = std::move(newEntries);
        }

        SKSE::log::info("Built map catalog with {} selectable worldspaces.", entries.size());

        for (const auto& entry : entries) {
            SKSE::log::debug(
                "Selectable map: name=\"{}\", editorID=\"{}\", FormID={:08X}, plugin=\"{}\"",
                entry.displayName, entry.editorID.empty() ? "<none>" : entry.editorID, entry.worldspace->GetFormID(), entry.pluginName
            );
        }
    }

    RE::TESWorldSpace* GetMapOwner(RE::TESWorldSpace* worldspace)
    {
        // Child worldspaces such as Whiterun can explicitly reuse their parent's map data.
        // The chooser should list the owning map once.
        auto* owner = worldspace;

        // Follow the parent only when this worldspace explicitly inherits the parent's map data.
        // The final pointer owns the displayed map.
        while (owner && owner->parentWorld && owner->parentUseFlags.any(RE::TESWorldSpace::ParentUseFlag::kUseMapData)) {
            owner = owner->parentWorld;
        }

        return owner;
    }

    std::vector<MapOption> GetOrderedOptions(RE::TESWorldSpace* currentWorldspace, RE::TESWorldSpace* selectedWorldspace)
    {
        // Copy a stable snapshot while holding a shared/read lock, then release the lock before sorting the private copy.
        std::shared_lock lock(entriesLock);
        auto result = entries;
        lock.unlock();

        std::ranges::sort(result, CompareMapOptions);

        auto* currentMap  = GetMapOwner(currentWorldspace);
        auto* selectedMap = GetMapOwner(selectedWorldspace);

        // Keep the useful status entries first; the remaining entries retain
        // the case-insensitive alphabetical ordering established above.

        const bool currentWasMoved = MoveToIndex(result, currentMap, 0);
        if (selectedMap != currentMap) {
            MoveToIndex(result, selectedMap, currentWasMoved ? 1 : 0);
        }

        return result;
    }
}
