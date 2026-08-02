#include "Config.h"
#include "MapSelection.h"

namespace WMS::MapSelection
{
    namespace
    {
        // This pointer is the player's persistent request
        // nullptr means no override.
        // atomic allows the input/UI and game threads to read or replace the pointer without a data race.
        std::atomic<RE::TESWorldSpace*> selectedWorldspace = nullptr;
    }

    void SelectDefault()
    {
        // release publishes the new selection to threads that later perform an acquire load.
        // Storing nullptr clears the request.
        selectedWorldspace.store(nullptr, std::memory_order_release);
    }

    void Select(RE::TESWorldSpace* worldspace)
    {
        selectedWorldspace.store(worldspace, std::memory_order_release);
    }

    void OnMapClosed()
    {
        // A non-persistent selection is consumed after a completed map session.
        if (!Config::GetPersistSelection() && GetSelectedWorldspace()) {
            SelectDefault();
            SKSE::log::info("Cleared one-shot map selection.");
        }
    }

    RE::TESWorldSpace* GetSelectedWorldspace()
    {
        // acquire ensures this thread observes the pointer published by store().
        return selectedWorldspace.load(std::memory_order_acquire);
    }
}
