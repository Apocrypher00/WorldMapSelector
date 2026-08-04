#pragma once

namespace WMS::Diagnostics
{
    void LogWorldspace(std::string_view label, const RE::TESWorldSpace* worldspace);
    void LogWorldspaceState();
}
