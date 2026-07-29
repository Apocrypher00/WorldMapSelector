#include "Diagnostics.h"
#include "WorldspaceOverride.h"

namespace WMS::Diagnostics
{
    void LogWorldspace(std::string_view label, const RE::TESWorldSpace* worldspace)
    {
        if (!worldspace) {
            SKSE::log::info("{} worldspace: <null>", label);
            return;
        }

        const auto* name = worldspace->GetName();
        const auto* editorID = worldspace->GetFormEditorID();

        SKSE::log::info(
            "{} worldspace: name=\"{}\", editorID=\"{}\", FormID={:08X}",
            label,
            name && name[0] ? name : "<unnamed>",
            editorID && editorID[0] ? editorID : "<none>",
            worldspace->GetFormID()
        );
    }

    namespace
    {
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

        class MenuEventSink final : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
        {
            public: RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override
            {
                if (!event || event->menuName != RE::MapMenu::MENU_NAME) {
                    return RE::BSEventNotifyControl::kContinue;
                }

                SKSE::log::info(
                    "MapMenu {}",
                    event->opening ? "opened" : "closed"
                );

                if (event->opening) {
                    WorldspaceOverride::BeginSession();
                    LogWorldspaceState();
                } else {
                    WorldspaceOverride::ResetSession();
                }

                return RE::BSEventNotifyControl::kContinue;
            }
        };
    }

    void RegisterMenuEventSink()
    {
        auto* ui = RE::UI::GetSingleton();
        if (!ui) {
            SKSE::log::error("Could not get the UI singleton.");
            return;
        }

        static MenuEventSink menuEventSink;
        ui->AddEventSink<RE::MenuOpenCloseEvent>(&menuEventSink);
        SKSE::log::info("Registered MapMenu event listener.");
    }
}
