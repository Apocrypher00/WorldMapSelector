#pragma once

namespace WMS::MapSwitchFlow
{
    // ConfigureOutsideMap records whether a completed choice should open the
    // map. ConfigureInsideMap records that the existing map must be replaced.
    void ConfigureOutsideMap(bool openAfterChoice);
    void ConfigureInsideMap();

    // Cancel discards the pending behavior. ApplyAfterChoice performs it after
    // a map is selected.
    void Cancel();
    void ApplyAfterChoice();

    // Returns true only when this close is the intermediate step of replacing
    // one open world map with another.
    bool OnMapMenuClosed();
}
