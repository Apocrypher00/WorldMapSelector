#pragma once

namespace WMS::MapChooser
{
    // RegisterInputSink installs the hotkey listener.
    // OnMapMenuClosed returns true only when the close is an intermediate step in a requested switch.
    bool RegisterInputSink();
    bool OnMapMenuClosed();
}
