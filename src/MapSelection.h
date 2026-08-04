#pragma once

namespace WMS::MapSelection
{
    void Select(RE::TESWorldSpace* worldspace);
    void SelectDefault();
    void OnMapClosed();
    RE::TESWorldSpace* GetSelectedWorldspace();
}
