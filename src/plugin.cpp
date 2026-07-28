#include "Diagnostics.h"
#include "MapMarkerOverride.h"
#include "WorldspaceOverride.h"

namespace
{
    void OnSKSEMessage(SKSE::MessagingInterface::Message* message)
    {
        if (message && (message->type == SKSE::MessagingInterface::kDataLoaded)) {
            WMS::WorldspaceOverride::LoadTestWorldspaces();
        }
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* skse)
{
    SKSE::Init(skse);

    SKSE::log::info("WorldMapSelector loaded successfully.");

    if (!WMS::WorldspaceOverride::Install()) {
        return false;
    }

    if (!WMS::MapMarkerOverride::Install()) {
        return false;
    }

    auto* messaging = SKSE::GetMessagingInterface();
    if (!messaging || !messaging->RegisterListener(OnSKSEMessage)) {
        SKSE::log::error("Could not register the SKSE message listener.");
        return false;
    }

    WMS::Diagnostics::RegisterMenuEventSink();
    return true;
}
