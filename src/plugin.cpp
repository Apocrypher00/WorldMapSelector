#include "PCH.h"
#include <MinHook.h>

#include <memory>

namespace
{
    RE::TESWorldSpace* tamrielWorldspace = nullptr;
    RE::TESWorldSpace* solstheimWorldspace = nullptr;
    RE::TESWorldSpace* selectedMapWorldspace = nullptr;

    RE::BSTArray<RE::ObjectRefHandle> selectedMapMarkerHandles;
    thread_local bool suppressPlayerMarkerLoop = false;

    void LogWorldspace(
        std::string_view label,
        const RE::TESWorldSpace* worldspace);

    using ResolveMapWorldSpace_t = RE::TESWorldSpace* (*)();
    ResolveMapWorldSpace_t originalResolveMapWorldSpace = nullptr;

    using CollectMapMarkerHandles_t = void (*)(
        RE::TESWorldSpace*,
        RE::BSTArray<RE::ObjectRefHandle>*,
        bool);

    using AddCurrentLocationMarker_t = void (*)(
        RE::BSTArray<RE::MapMenuMarker>*,
        RE::NiPoint3*);
    AddCurrentLocationMarker_t originalAddCurrentLocationMarker = nullptr;

    using AddMarkerFromHandle_t = std::uint64_t (*)(
        RE::BSTArray<RE::MapMenuMarker>**,
        RE::ObjectRefHandle*);
    AddMarkerFromHandle_t originalAddMarkerFromHandle = nullptr;

    RE::TESWorldSpace* ResolveMapWorldSpaceHook()
    {
        auto* result = originalResolveMapWorldSpace();
        selectedMapWorldspace = nullptr;

        if (result == tamrielWorldspace && solstheimWorldspace) {
            selectedMapWorldspace = solstheimWorldspace;
            SKSE::log::info(
                "Map worldspace override: Tamriel -> Solstheim");
            return selectedMapWorldspace;
        }

        if (result == solstheimWorldspace && tamrielWorldspace) {
            selectedMapWorldspace = tamrielWorldspace;
            SKSE::log::info(
                "Map worldspace override: Solstheim -> Tamriel");
            return selectedMapWorldspace;
        }

        return result;
    }

    void AddCurrentLocationMarkerHook(
        RE::BSTArray<RE::MapMenuMarker>* mapMarkers,
        RE::NiPoint3* playerMarkerPosition)
    {
        suppressPlayerMarkerLoop = false;

        originalAddCurrentLocationMarker(
            mapMarkers,
            playerMarkerPosition);

        if (!selectedMapWorldspace || !mapMarkers) {
            return;
        }

        static REL::Relocation<CollectMapMarkerHandles_t>
            collectMapMarkerHandles{ REL::ID(20536) };

        selectedMapMarkerHandles.clear();
        collectMapMarkerHandles(
            selectedMapWorldspace,
            std::addressof(selectedMapMarkerHandles),
            false);

        const auto markerCountBefore = mapMarkers->size();
        auto* destination = mapMarkers;
        for (auto& handle : selectedMapMarkerHandles) {
            originalAddMarkerFromHandle(
                std::addressof(destination),
                std::addressof(handle));
        }
        const auto markersAdded =
            mapMarkers->size() - markerCountBefore;

        suppressPlayerMarkerLoop = true;

        SKSE::log::info(
            "Collected {} handles and added {} markers for selected "
            "map worldspace {:08X}.",
            selectedMapMarkerHandles.size(),
            markersAdded,
            selectedMapWorldspace->GetFormID());
    }

    std::uint64_t AddMarkerFromHandleHook(
        RE::BSTArray<RE::MapMenuMarker>** destination,
        RE::ObjectRefHandle* handle)
    {
        if (suppressPlayerMarkerLoop) {
            suppressPlayerMarkerLoop = false;
            return 0;
        }

        return originalAddMarkerFromHandle(destination, handle);
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

    bool InstallMarkerHooks()
    {
        REL::Relocation<std::uintptr_t> addCurrentLocationMarker{
            REL::ID(53076)
        };
        REL::Relocation<std::uintptr_t> addMarkerFromHandle{
            REL::ID(53126)
        };

        const auto createCurrentLocationStatus = MH_CreateHook(
            reinterpret_cast<void*>(addCurrentLocationMarker.address()),
            reinterpret_cast<void*>(AddCurrentLocationMarkerHook),
            reinterpret_cast<void**>(&originalAddCurrentLocationMarker));

        if (createCurrentLocationStatus != MH_OK) {
            SKSE::log::error(
                "Current-location marker hook creation failed: {}",
                static_cast<int>(createCurrentLocationStatus));
            return false;
        }

        const auto createMarkerStatus = MH_CreateHook(
            reinterpret_cast<void*>(addMarkerFromHandle.address()),
            reinterpret_cast<void*>(AddMarkerFromHandleHook),
            reinterpret_cast<void**>(&originalAddMarkerFromHandle));

        if (createMarkerStatus != MH_OK) {
            SKSE::log::error(
                "Marker-from-handle hook creation failed: {}",
                static_cast<int>(createMarkerStatus));
            return false;
        }

        const auto enableCurrentLocationStatus = MH_EnableHook(
            reinterpret_cast<void*>(addCurrentLocationMarker.address()));

        if (enableCurrentLocationStatus != MH_OK) {
            SKSE::log::error(
                "Current-location marker hook activation failed: {}",
                static_cast<int>(enableCurrentLocationStatus));
            return false;
        }

        const auto enableMarkerStatus = MH_EnableHook(
            reinterpret_cast<void*>(addMarkerFromHandle.address()));

        if (enableMarkerStatus != MH_OK) {
            SKSE::log::error(
                "Marker-from-handle hook activation failed: {}",
                static_cast<int>(enableMarkerStatus));
            return false;
        }

        SKSE::log::info(
            "Installed MapMenu marker detours at {:X} and {:X}.",
            addCurrentLocationMarker.address(),
            addMarkerFromHandle.address());
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

    if (!InstallMarkerHooks()) {
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
