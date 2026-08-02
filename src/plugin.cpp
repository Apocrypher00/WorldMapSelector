#include "Config.h"
#include "MapChooser.h"
#include "MapMenuEvents.h"
#include "MapMarkerOverride.h"
#include "RoutedMarkerOverride.h"
#include "WorldspaceCatalog.h"
#include "WorldspaceOverride.h"

#include <MinHook.h>

namespace
{
    // SKSE calls this function after each stage of game startup.
    // The message parameter points to a small object describing which stage just finished.
    void OnSKSEMessage(SKSE::MessagingInterface::Message* message)
    {
		// Return immediately if the message pointer is null.
        if (!message) { return; }

        if (message->type == SKSE::MessagingInterface::kInputLoaded) {
            // Skyrim's input devices and input event source now exist,
            // so the chooser can safely register its keyboard listener.
            WMS::MapChooser::RegisterInputSink();
        } else if (message->type == SKSE::MessagingInterface::kDataLoaded) {
            // Every active plugin has finished loading and its forms have their final runtime FormIDs,
            // so it is now safe to build the map catalogue.
            WMS::WorldspaceCatalog::Build();
        }
    }
}

// SKSE calls this exported entry point when it loads the DLL.
// Returning true accepts the plugin; returning false tells SKSE that initialization failed.
SKSEPluginLoad(const SKSE::LoadInterface* skse)
{
    // Save the interfaces supplied through the skse pointer,
    // and initialize CommonLib's SKSE API and plugin logger.
    SKSE::Init(skse);

    // Read all settings once for this game process.
    WMS::Config::Load();

    // Initialize the hooking library once before any override module creates
    // its detours.
    const auto minHookStatus = MH_Initialize();
    if (minHookStatus != MH_OK &&
        minHookStatus != MH_ERROR_ALREADY_INITIALIZED) {
        SKSE::log::error(
            "MinHook initialization failed: {}",
            static_cast<int>(minHookStatus));
        return false;
    }

    // Install the resolver detour.
    // If Install() returns false, the detour could not be created/enabled, so stop initialization and report failure to SKSE.
    if (!WMS::WorldspaceOverride::Install()) { return false; }

    // Install the two ordinary marker detours.
    if (!WMS::MapMarkerOverride::Install()) { return false; }

    // Install the four routed quest/custom-destination marker detours.
    if (!WMS::RoutedMarkerOverride::Install()) { return false; }

    // RegisterListener stores OnSKSEMessage as the callback SKSE will invoke.
    // auto* asks C++ to infer the pointer type returned by this function.
    auto* messaging = SKSE::GetMessagingInterface();
    if (!messaging || !messaging->RegisterListener(OnSKSEMessage)) {
        SKSE::log::error("Could not register the SKSE message listener.");
        return false;
    }

    // Subscribe to MapMenu open/close events.
    // These events freeze and clear the selected-map session and coordinate close/reopen map switches.
    if (!WMS::MapMenuEvents::Register()) { return false; }

    // All required initialization above completed without returning false.
    SKSE::log::info("WorldMapSelector loaded successfully.");
    return true;
}
