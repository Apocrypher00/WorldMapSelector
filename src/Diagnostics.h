#pragma once

namespace WMS::Diagnostics
{
    // string_view borrows label text without copying it.
    // The caller must keep that text alive for the duration of the call.
    void LogWorldspace(std::string_view label, const RE::TESWorldSpace* worldspace);
    void LogWorldspaceState();
}
