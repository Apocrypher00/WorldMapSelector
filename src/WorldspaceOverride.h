#pragma once

namespace WMS::WorldspaceOverride
{
    bool CreateHook();
    void BeginSession();
    void ResetSession();
    RE::TESWorldSpace* GetActualMapWorldspace();
    RE::TESWorldSpace* GetSelectedMapWorldspace();
}
