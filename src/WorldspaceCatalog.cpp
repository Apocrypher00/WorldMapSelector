#include "WorldspaceCatalog.h"
#include "Utilities.h"

#include <charconv>
#include <shared_mutex>

namespace WMS::WorldspaceCatalog
{
    namespace
    {
        // entries is shared by the game-start builder and later UI/command
        // readers. shared_mutex permits multiple readers but only one writer.
        std::vector<MapOption> entries;
        std::shared_mutex entriesLock;

        std::string_view Trim(std::string_view value)
        {
            const auto first = value.find_first_not_of(" \t\r\n");
            if (first == std::string_view::npos) {
                return {};
            }

            const auto last = value.find_last_not_of(" \t\r\n");
            // substr returns another non-owning view into the original text.
            return value.substr(first, last - first + 1);
        }

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

        std::optional<RE::FormID> ParseRuntimeFormID(std::string_view text)
        {
            text = Trim(text);
            if (text.starts_with("0x") || text.starts_with("0X")) {
                text.remove_prefix(2);
            }

            RE::FormID value = 0;
            // Structured binding assigns the two fields returned by from_chars to the local names end and error.
            // Base 16 treats the text as hex.
            const auto [end, error] = std::from_chars(
                text.data(),
                text.data() + text.size(),
                value,
                16);

            if (error != std::errc{} ||
                end != text.data() + text.size()) {
                return std::nullopt;
            }

            return value;
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
            SKSE::log::info(
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
            [&](RE::TESWorldSpace* worldspace, std::size_t index) {
                if (!worldspace || index >= result.size()) {
                    return;
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
                }
            };

        auto* currentMap =
            ResolveMapOwner(currentWorldspace);
        auto* selectedMap =
            ResolveMapOwner(selectedWorldspace);

        // Keep the useful status entries first; the remaining entries retain
        // the case-insensitive alphabetical ordering established above.

        moveToIndex(currentMap, 0);
        if (selectedMap != currentMap) {
            moveToIndex(selectedMap, currentMap ? 1 : 0);
        }

        return result;
    }

    SelectionResult ResolveSelection(std::string_view identifier)
    {
        identifier = Trim(identifier);

        if (identifier.empty() ||
            Utilities::EqualsIgnoreCase(identifier, "Default")) {
            return { .isDefault = true };
        }

        std::shared_lock lock(entriesLock);

        // This if-with-initializer keeps formID scoped to the numeric lookup.
        // optional converts to true only when parsing produced a value.
        if (const auto formID = ParseRuntimeFormID(identifier)) {
            for (const auto& entry : entries) {
                // * extracts the value stored inside the optional.
                if (entry.worldspace->GetFormID() == *formID) {
                    return { .worldspace = entry.worldspace };
                }
            }

            return {
                .error = fmt::format(
                    "FormID {:08X} is not a selectable map.",
                    *formID)
            };
        }

        RE::TESWorldSpace* match = nullptr;
        for (const auto& entry : entries) {
            if (!entry.editorID.empty() &&
                Utilities::EqualsIgnoreCase(entry.editorID, identifier)) {
                if (match) {
                    return {
                        .error = fmt::format(
                            "EditorID \"{}\" is ambiguous; use "
                            "its runtime FormID.",
                            identifier)
                    };
                }
                match = entry.worldspace;
            }
        }

        if (!match) {
            return {
                .error = fmt::format(
                    "EditorID \"{}\" is not a selectable map.",
                    identifier)
            };
        }

        return { .worldspace = match };
    }
}
