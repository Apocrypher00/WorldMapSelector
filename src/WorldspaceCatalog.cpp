#include "WorldspaceCatalog.h"

#include <charconv>
#include <shared_mutex>

namespace WMS::WorldspaceCatalog
{
    namespace
    {
        std::vector<MapOption> entries;
        std::shared_mutex entriesLock;

        bool EqualsIgnoreCase(std::string_view left, std::string_view right)
        {
            return left.size() == right.size() &&
                   _strnicmp(left.data(), right.data(), left.size()) == 0;
        }

        std::string_view Trim(std::string_view value)
        {
            const auto first = value.find_first_not_of(" \t\r\n");
            if (first == std::string_view::npos) {
                return {};
            }

            const auto last = value.find_last_not_of(" \t\r\n");
            return value.substr(first, last - first + 1);
        }

        std::string SafeText(const char* text, std::string_view fallback)
        {
            return text && text[0] ? text : std::string(fallback);
        }

        RE::TESWorldSpace* ResolveMapOwner(RE::TESWorldSpace* worldspace)
        {
            auto* owner = worldspace;

            while (owner &&
                   owner->parentWorld &&
                   owner->parentUseFlags.any(
                       RE::TESWorldSpace::ParentUseFlag::kUseMapData)) {
                owner = owner->parentWorld;
            }

            return owner;
        }

        bool IsMapCandidate(RE::TESWorldSpace* worldspace)
        {
            if (!worldspace || ResolveMapOwner(worldspace) != worldspace) {
                return false;
            }

            const auto& map = worldspace->worldMapData;
            return map.nwCellX != map.seCellX ||
                   map.nwCellY != map.seCellY;
        }

        std::optional<RE::FormID> ParseRuntimeFormID(
            std::string_view text)
        {
            text = Trim(text);
            if (text.starts_with("0x") || text.starts_with("0X")) {
                text.remove_prefix(2);
            }

            RE::FormID value = 0;
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

    void Build()
    {
        auto* dataHandler = RE::TESDataHandler::GetSingleton();
        if (!dataHandler) {
            SKSE::log::error(
                "Could not get the data handler for the map catalog.");
            return;
        }

        std::vector<MapOption> newEntries;

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
            std::unique_lock lock(entriesLock);
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
        std::shared_lock lock(entriesLock);
        auto result = entries;
        lock.unlock();

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

        const auto moveToIndex =
            [&](RE::TESWorldSpace* worldspace, std::size_t index) {
                if (!worldspace || index >= result.size()) {
                    return;
                }

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
            EqualsIgnoreCase(identifier, "Default")) {
            return { .isDefault = true };
        }

        std::shared_lock lock(entriesLock);

        if (const auto formID = ParseRuntimeFormID(identifier)) {
            for (const auto& entry : entries) {
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
                EqualsIgnoreCase(entry.editorID, identifier)) {
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
