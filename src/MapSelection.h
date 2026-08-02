#pragma once

namespace WMS::MapSelection
{
    // A null worldspace, exposed through SelectDefault(), represents vanilla
    // behavior rather than a particular map.
    void SelectDefault();
    void Select(RE::TESWorldSpace* worldspace);
    void OnMapClosed();
    RE::TESWorldSpace* GetSelectedWorldspace();
}
