#pragma once

namespace WMS::WorldspaceOverride
{
    bool Install();
    void BeginSession();
    void ResetSession();
    RE::TESWorldSpace* GetSelectedMapWorldspace();
}
