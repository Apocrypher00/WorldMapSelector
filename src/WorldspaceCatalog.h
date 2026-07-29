#pragma once

namespace WMS::WorldspaceCatalog
{
    struct SelectionResult
    {
        RE::TESWorldSpace* worldspace = nullptr;
        bool isDefault = false;
        std::string error;
    };

    void Build();
    SelectionResult ResolveSelection(std::string_view identifier);
}
