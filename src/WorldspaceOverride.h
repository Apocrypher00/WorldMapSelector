#pragma once

namespace WMS::WorldspaceOverride
{
    bool Install();
    void LoadTestWorldspaces();
    RE::TESWorldSpace* GetSelectedMapWorldspace();
}
