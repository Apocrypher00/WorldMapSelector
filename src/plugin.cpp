#include "PCH.h"

namespace
{
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
