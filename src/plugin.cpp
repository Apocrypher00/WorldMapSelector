#include "PCH.h"
#include <MinHook.h>

namespace
{
    RE::TESWorldSpace* tamrielWorldspace = nullptr;
    RE::TESWorldSpace* solstheimWorldspace = nullptr;

    void LogWorldspace(
        std::string_view label,
        const RE::TESWorldSpace* worldspace);

    using ResolveMapWorldSpace_t = RE::TESWorldSpace* (*)();
    ResolveMapWorldSpace_t originalResolveMapWorldSpace = nullptr;

    RE::TESWorldSpace* ResolveMapWorldSpaceHook()
    {
        auto* result = originalResolveMapWorldSpace();

        if (result == tamrielWorldspace && solstheimWorldspace) {
            SKSE::log::info(
                "Map worldspace override: Tamriel -> Solstheim");
            return solstheimWorldspace;
        }

        if (result == solstheimWorldspace && tamrielWorldspace) {
            SKSE::log::info(
                "Map worldspace override: Solstheim -> Tamriel");
            return tamrielWorldspace;
        }

        return result;
    }

    bool InstallResolverHook()
    {
        REL::Relocation<std::uintptr_t> resolver{
            REL::RelocationID(52260, 53150)
        };

        const auto initializeStatus = MH_Initialize();
        if (initializeStatus != MH_OK &&
            initializeStatus != MH_ERROR_ALREADY_INITIALIZED) {
            SKSE::log::error(
                "MinHook initialization failed: {}",
                static_cast<int>(initializeStatus));
            return false;
        }

        const auto createStatus = MH_CreateHook(
            reinterpret_cast<void*>(resolver.address()),
            reinterpret_cast<void*>(ResolveMapWorldSpaceHook),
            reinterpret_cast<void**>(&originalResolveMapWorldSpace));

        if (createStatus != MH_OK) {
            SKSE::log::error(
                "Resolver hook creation failed: {}",
                static_cast<int>(createStatus));
            return false;
        }

        const auto enableStatus =
            MH_EnableHook(reinterpret_cast<void*>(resolver.address()));

        if (enableStatus != MH_OK) {
            SKSE::log::error(
                "Resolver hook activation failed: {}",
                static_cast<int>(enableStatus));
            return false;
        }

        SKSE::log::info(
            "Installed WMS_ResolveMapWorldSpace detour at {:X}.",
            resolver.address());
        return true;
    }

    void LoadTestWorldspaces()
    {
        auto* dataHandler = RE::TESDataHandler::GetSingleton();
        if (!dataHandler) {
            SKSE::log::error("Could not get the data handler.");
            return;
        }

        tamrielWorldspace =
            dataHandler->LookupForm<RE::TESWorldSpace>(
                0x00003C, "Skyrim.esm");
        solstheimWorldspace =
            dataHandler->LookupForm<RE::TESWorldSpace>(
                0x000800, "Dragonborn.esm");

        LogWorldspace("Loaded Tamriel test", tamrielWorldspace);
        LogWorldspace("Loaded Solstheim test", solstheimWorldspace);
    }

    void LogWorldspace(
        std::string_view label,
        const RE::TESWorldSpace* worldspace)
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
            worldspace->GetFormID());
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

        const auto mapMenu = ui->GetMenu<RE::MapMenu>();

        if (!mapMenu) {
            SKSE::log::warn(
                "MapMenu instance was unavailable during the open event.");
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
                LogWorldspaceState();
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

    void OnSKSEMessage(SKSE::MessagingInterface::Message* message)
    {
        if (message &&
            message->type == SKSE::MessagingInterface::kDataLoaded) {
            LoadTestWorldspaces();
        }
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* skse)
{
    SKSE::Init(skse);

    SKSE::log::info("WorldMapSelector loaded successfully.");

    if (!InstallResolverHook()) {
        return false;
    }

    auto* messaging = SKSE::GetMessagingInterface();
    if (!messaging || !messaging->RegisterListener(OnSKSEMessage)) {
        SKSE::log::error("Could not register the SKSE message listener.");
        return false;
    }

    RegisterMenuEventSink();

    return true;
}
