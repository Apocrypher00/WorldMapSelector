#include "Diagnostics.h"

namespace WMS::Diagnostics
{
    // string_view borrows label text without copying it.
    // The caller must keep that text alive for the duration of the call.
    void LogWorldspace(std::string_view label, const RE::TESWorldSpace* worldspace)
    {
        if (!worldspace) {
            SKSE::log::debug("{} worldspace: <null>", label);
            return;
        }

        const auto* name     = worldspace->GetName();
        const auto* editorID = worldspace->GetFormEditorID();

        const char* formattedName     = name && name[0] ? name : "<unnamed>";
		const char* formattedEditorId = editorID && editorID[0] ? editorID : "<none>";

        SKSE::log::debug(
            "{} worldspace: name=\"{}\", editorID=\"{}\", FormID={:08X}",
            label, formattedName, formattedEditorId, worldspace->GetFormID()
        );
    }

    void LogWorldspaceState()
    {
        const auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player) {
            SKSE::log::error("Could not get the player singleton.");
            return;
        }

        LogWorldspace("Player", player->GetWorldspace());

        auto* ui = RE::UI::GetSingleton();
        if (!ui) {
            SKSE::log::error("Could not get the UI singleton.");
            return;
        }

        // <RE::MapMenu> is a template argument.
        // It asks GetMenu for this specific menu type instead of returning an untyped menu.
        const auto mapMenu = ui->GetMenu<RE::MapMenu>();
        if (!mapMenu) {
            SKSE::log::warn("MapMenu instance was unavailable during the open event.");
            return;
        }

        const auto* runtimeData = mapMenu->GetRuntimeData2();
        if (!runtimeData) {
            SKSE::log::warn("MapMenu runtime data was unavailable.");
            return;
        }

        LogWorldspace("MapMenu", runtimeData->worldSpace);
        LogWorldspace("MapCamera", runtimeData->camera.worldSpace);
    }
}
