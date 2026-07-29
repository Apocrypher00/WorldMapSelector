#pragma once

namespace WMS::MapSelection
{
    void SelectDefault();
    void Select(RE::TESWorldSpace* worldspace);
    void OnMapClosed();
    RE::TESWorldSpace* GetSelectedWorldspace();
}
