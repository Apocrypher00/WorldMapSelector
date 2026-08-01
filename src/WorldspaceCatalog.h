#pragma once

namespace WMS::WorldspaceCatalog
{
    struct MapOption
    {
        RE::TESWorldSpace* worldspace = nullptr;
        std::string displayName;
        std::string editorID;
        std::string pluginName;
    };

    struct SelectionResult
    {
        RE::TESWorldSpace* worldspace = nullptr;
        bool isDefault = false;
        std::string error;
    };

    void Build();
    RE::TESWorldSpace* GetMapOwner(RE::TESWorldSpace* worldspace);
    std::vector<MapOption> GetOrderedOptions(
        RE::TESWorldSpace* currentWorldspace,
        RE::TESWorldSpace* selectedWorldspace);
    SelectionResult ResolveSelection(std::string_view identifier);
}
