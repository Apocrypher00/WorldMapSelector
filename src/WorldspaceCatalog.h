#pragma once

namespace WMS::WorldspaceCatalog
{
    // Each option owns its display strings but only borrows the TESWorldSpace;
    // Skyrim owns that form for the lifetime of the loaded game data.
    struct MapOption
    {
        RE::TESWorldSpace* worldspace = nullptr;
        std::string displayName;
        std::string editorID;
        std::string pluginName;
    };

    void Build();
    RE::TESWorldSpace* GetMapOwner(RE::TESWorldSpace* worldspace);
    std::vector<MapOption> GetOrderedOptions(
        RE::TESWorldSpace* currentWorldspace,
        RE::TESWorldSpace* selectedWorldspace);
}
