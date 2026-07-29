#pragma once

namespace WMS::WorldspaceOverride
{
    bool Install();
    void BeginSession();
    void ResetSession();
    RE::TESWorldSpace* GetActualMapWorldspace();
    RE::TESWorldSpace* GetSelectedMapWorldspace();
}
