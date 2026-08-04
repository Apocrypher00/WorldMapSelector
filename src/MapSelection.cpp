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

    // release publishes the new selection to threads that later perform an acquire load.
    void Select(RE::TESWorldSpace* worldspace) { selectedWorldspace.store(worldspace, std::memory_order_release); }

    // Storing nullptr clears the request.
    // A null worldspace represents vanilla behavior rather than a particular map.
    void SelectDefault() { Select(nullptr); }

    // A non-persistent selection is consumed after a completed map session.
    void OnMapClosed()
    {
        if (!Config::GetPersistSelection() && GetSelectedWorldspace()) {
            SelectDefault();
            SKSE::log::info("Cleared one-shot map selection.");
        }
    }

    // acquire ensures this thread observes the pointer published by store().
    RE::TESWorldSpace* GetSelectedWorldspace() { return selectedWorldspace.load(std::memory_order_acquire); }
}
