#include "Config.h"
#include "MapSelection.h"

namespace WMS::MapSelection
{
    namespace
    {
        std::atomic<RE::TESWorldSpace*> selectedWorldspace = nullptr;
    }

    void SelectDefault()
    {
        selectedWorldspace.store(
            nullptr,
            std::memory_order_release);
    }

    void Select(RE::TESWorldSpace* worldspace)
    {
        selectedWorldspace.store(
            worldspace,
            std::memory_order_release);
    }

    void OnMapClosed()
    {
        if (!Config::GetPersistSelection() &&
            GetSelectedWorldspace()) {
            SelectDefault();
            SKSE::log::info(
                "Cleared one-shot map selection.");
        }
    }

    RE::TESWorldSpace* GetSelectedWorldspace()
    {
        return selectedWorldspace.load(
            std::memory_order_acquire);
    }
}
