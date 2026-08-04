#pragma once

namespace WMS::MapSwitchFlow
{
    void ConfigureOutsideMap(bool openAfterChoice);
    void ConfigureInsideMap();
    void Cancel();
    void ApplyAfterChoice();
    bool OnMapMenuClosed();
}
