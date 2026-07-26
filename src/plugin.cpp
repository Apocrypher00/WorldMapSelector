#include "PCH.h"

namespace
{
    void LogPlayerWorldspace()
    {
        const auto* player = RE::PlayerCharacter::GetSingleton();

        if (!player) {
            SKSE::log::error("Could not get the player singleton.");
            return;
        }

        const auto* worldspace = player->GetWorldspace();

        if (!worldspace) {
            SKSE::log::info("Player is not in a worldspace.");
            return;
        }

        const auto* name = worldspace->GetName();
        const auto* editorID = worldspace->GetFormEditorID();

        SKSE::log::info(
            "Player worldspace: name=\"{}\", editorID=\"{}\", FormID={:08X}",
            name && name[0] ? name : "<unnamed>",
            editorID && editorID[0] ? editorID : "<none>",
            worldspace->GetFormID());
    }

    class MenuEventSink final :
        public RE::BSTEventSink<RE::MenuOpenCloseEvent>
    {
    public:
        RE::BSEventNotifyControl ProcessEvent(
            const RE::MenuOpenCloseEvent* event,
            RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override
        {
            if (!event || event->menuName != RE::MapMenu::MENU_NAME) {
                return RE::BSEventNotifyControl::kContinue;
            }

            SKSE::log::info(
                "MapMenu {}",
                event->opening ? "opened" : "closed");

            if (event->opening) {
                LogPlayerWorldspace();
            }

            return RE::BSEventNotifyControl::kContinue;
        }
    };

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

SKSEPluginLoad(const SKSE::LoadInterface* skse)
{
    SKSE::Init(skse);

    SKSE::log::info("WorldMapSelector loaded successfully.");

    RegisterMenuEventSink();

    return true;
}
