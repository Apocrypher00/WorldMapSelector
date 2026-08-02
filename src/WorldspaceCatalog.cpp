#include "WorldspaceCatalog.h"

#include <shared_mutex>

namespace WMS::WorldspaceCatalog
{
    namespace
    {
        // entries is shared by the game-start builder and later UI/command
        // readers. shared_mutex permits multiple readers but only one writer.
        std::vector<MapOption> entries;
        std::shared_mutex entriesLock;

        // Convert Skyrim's borrowed C string into an owned std::string.
        std::string SafeText(const char* text, std::string_view fallback)
        {
            return text && text[0] ? text : std::string(fallback);
        }

        RE::TESWorldSpace* ResolveMapOwner(RE::TESWorldSpace* worldspace)
        {
            // Child worldspaces such as Whiterun can explicitly reuse their parent's map data.
            // The chooser should list the owning map once.
            auto* owner = worldspace;

            // Follow the parent only when this worldspace explicitly inherits
            // the parent's map data. The final pointer owns the displayed map.
            while (owner && owner->parentWorld && owner->parentUseFlags.any(
                RE::TESWorldSpace::ParentUseFlag::kUseMapData
            )) {
                owner = owner->parentWorld;
            }

            return owner;
        }

        bool IsMapCandidate(RE::TESWorldSpace* worldspace)
        {
            if (!worldspace || ResolveMapOwner(worldspace) != worldspace) {
                return false;
            }

            // A usable world map needs non-degenerate cell bounds. This also
            // admits map data supplied to normally mapless worlds by mods.
            const auto& map = worldspace->worldMapData;
            return map.nwCellX != map.seCellX ||
                   map.nwCellY != map.seCellY;
        }

    }

    // Called after SKSE's DataLoaded message, when forms from every active plugin are available and their runtime FormIDs are final.
    void Build()
    {
        auto* dataHandler = RE::TESDataHandler::GetSingleton();
        if (!dataHandler) {
            SKSE::log::error(
                "Could not get the data handler for the map catalog.");
            return;
        }

        std::vector<MapOption> newEntries;

        // Range-based for visits every loaded TESWorldSpace form.
        for (auto* worldspace :
             dataHandler->GetFormArray<RE::TESWorldSpace>()) {
            if (!IsMapCandidate(worldspace)) {
                continue;
            }

            const auto* file = worldspace->GetFile(0);
            if (!file) {
                SKSE::log::warn(
                    "Ignoring dynamic map candidate {:08X}; it has no "
                    "originating plugin.",
                    worldspace->GetFormID());
                continue;
            }

            // Designated initializers name each MapOption field being filled.
            newEntries.push_back({
                .worldspace = worldspace,
                .displayName =
                    SafeText(worldspace->GetName(), "<unnamed>"),
                .editorID =
                    SafeText(worldspace->GetFormEditorID(), ""),
                .pluginName = std::string(file->GetFilename())
            });
        }

        {
            // unique_lock excludes readers while replacing the shared catalogue.
            std::unique_lock lock(entriesLock);
            // move transfers the vector's allocation instead of copying every entry.
            entries = std::move(newEntries);
        }

        SKSE::log::info(
            "Built map catalog with {} selectable worldspaces.",
            entries.size());

        for (const auto& entry : entries) {
            SKSE::log::debug(
                "Selectable map: name=\"{}\", editorID=\"{}\", "
                "FormID={:08X}, plugin=\"{}\"",
                entry.displayName,
                entry.editorID.empty() ? "<none>" : entry.editorID,
                entry.worldspace->GetFormID(),
                entry.pluginName);
        }
    }

    RE::TESWorldSpace* GetMapOwner(RE::TESWorldSpace* worldspace)
    {
        return ResolveMapOwner(worldspace);
    }

    std::vector<MapOption> GetOrderedOptions(
        RE::TESWorldSpace* currentWorldspace,
        RE::TESWorldSpace* selectedWorldspace)
    {
        // Copy a stable snapshot while holding a shared/read lock, then release
        // the lock before sorting the private copy.
        std::shared_lock lock(entriesLock);
        auto result = entries;
        lock.unlock();

        // The lambda is an unnamed comparison function passed into sort.
        std::ranges::sort(
            result,
            [](const MapOption& left, const MapOption& right) {
                const auto nameComparison =
                    _stricmp(
                        left.displayName.c_str(),
                        right.displayName.c_str());
                if (nameComparison != 0) {
                    return nameComparison < 0;
                }

                return _stricmp(
                           left.editorID.c_str(),
                           right.editorID.c_str()) < 0;
            });

        // Capture [&] lets this lambda use result by reference. It moves a
        // requested worldspace to index without disturbing more items than necessary.
        const auto moveToIndex =
            [&](RE::TESWorldSpace* worldspace, std::size_t index) -> bool {
                if (!worldspace || index >= result.size()) {
                    return false;
                }

                // next advances an iterator; find_if searches from that point;
                // rotate moves the found entry into the destination position.
                const auto destination =
                    std::next(result.begin(), index);
                const auto option = std::find_if(
                    destination,
                    result.end(),
                    [&](const auto& candidate) {
                        return candidate.worldspace == worldspace;
                    });
                if (option != result.end()) {
                    std::rotate(
                        destination,
                        option,
                        std::next(option));
                    return true;
                }

                return false;
            };

        auto* currentMap =
            ResolveMapOwner(currentWorldspace);
        auto* selectedMap =
            ResolveMapOwner(selectedWorldspace);

        // Keep the useful status entries first; the remaining entries retain
        // the case-insensitive alphabetical ordering established above.

        const bool currentWasMoved = moveToIndex(currentMap, 0);
        if (selectedMap != currentMap) {
            moveToIndex(selectedMap, currentWasMoved ? 1 : 0);
        }

        return result;
    }
}
