#pragma once

namespace WMS::MapSelection
{
    void SelectDefault();
    void Select(RE::TESWorldSpace* worldspace);
    RE::TESWorldSpace* GetSelectedWorldspace();
}
