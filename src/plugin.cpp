#include "Config.h"
#include "Diagnostics.h"
#include "MapChooser.h"
#include "MapMarkerOverride.h"
#include "WorldspaceCatalog.h"
#include "WorldspaceOverride.h"

namespace
{
    void OnSKSEMessage(SKSE::MessagingInterface::Message* message)
    {
        if (!message) {
            return;
        }

        if (message->type ==
            SKSE::MessagingInterface::kInputLoaded) {
            WMS::MapChooser::RegisterInputSink();
        } else if (message->type ==
                   SKSE::MessagingInterface::kDataLoaded) {
            WMS::WorldspaceCatalog::Build();
        }
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* skse)
{
    SKSE::Init(skse);
    WMS::Config::Load();

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
