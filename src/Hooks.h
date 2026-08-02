#pragma once

namespace WMS::Hooks
{
    // Own MinHook's process-wide lifecycle. Override modules create disabled
    // hooks; EnableAll activates them together after every creation succeeds.
    bool Initialize();
    bool EnableAll();
    void Reset();
}
