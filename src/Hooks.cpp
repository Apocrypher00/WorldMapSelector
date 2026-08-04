#include "Hooks.h"

namespace WMS::Hooks
{
    bool Initialize()
    {
        const auto status = MH_Initialize();
        if (status == MH_OK || status == MH_ERROR_ALREADY_INITIALIZED) {
            return true;
        } else {
            SKSE::log::error("MinHook initialization failed: {}", MH_StatusToString(status));
            return false;
        }
    }

    bool EnableAll()
    {
        const auto status = MH_EnableHook(MH_ALL_HOOKS);
        if (status == MH_OK) {
            SKSE::log::info("Enabled all WorldMapSelector detours.");
            return true;
        } else {
            SKSE::log::error("MinHook could not enable all detours: {}", MH_StatusToString(status));
            return false;
        }
    }

    void Reset()
    {
        // Uninitialize disables every enabled hook, removes every created hook, and releases MinHook's internal resources.
        const auto status = MH_Uninitialize();
        if (status != MH_OK && status != MH_ERROR_NOT_INITIALIZED) {
            SKSE::log::error("MinHook cleanup failed: {}", MH_StatusToString(status));
        }
    }
}
