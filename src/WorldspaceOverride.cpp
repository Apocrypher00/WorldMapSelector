#include "Config.h"
#include "Diagnostics.h"
#include "WorldspaceOverride.h"

#include <MinHook.h>

namespace WMS::WorldspaceOverride
{
    namespace
    {
        RE::TESWorldSpace* tamrielWorldspace = nullptr;
        RE::TESWorldSpace* solstheimWorldspace = nullptr;
        std::atomic<RE::TESWorldSpace*> selectedMapWorldspace = nullptr;
        std::atomic<Config::MapSelection> lastSelection =
            Config::MapSelection::kActual;

        using ResolveMapWorldSpace_t = RE::TESWorldSpace* (*)();
        ResolveMapWorldSpace_t originalResolveMapWorldSpace = nullptr;

        RE::TESWorldSpace* ResolveMapWorldSpaceHook()
        {
            auto* actualWorldspace = originalResolveMapWorldSpace();
            const auto selection = Config::ReadMapSelection();
            const auto previousSelection = lastSelection.exchange(selection);

            if (selection != previousSelection) {
                SKSE::log::info(
                    "Map selection mode: {}",
                    Config::ToString(selection));
            }

            RE::TESWorldSpace* selectedWorldspace = nullptr;
            if (selection == Config::MapSelection::kOpposite) {
                if (actualWorldspace == tamrielWorldspace) {
                    selectedWorldspace = solstheimWorldspace;
                } else if (actualWorldspace == solstheimWorldspace) {
                    selectedWorldspace = tamrielWorldspace;
                }
            }

            selectedMapWorldspace.store(
                selectedWorldspace,
                std::memory_order_release);

            if (!selectedWorldspace) {
                return actualWorldspace;
            }

            SKSE::log::info(
                "Map worldspace override: {:08X} -> {:08X}",
                actualWorldspace->GetFormID(),
                selectedWorldspace->GetFormID());
            return selectedWorldspace;
        }
    }

    RE::TESWorldSpace* GetSelectedMapWorldspace()
    {
        return selectedMapWorldspace.load(std::memory_order_acquire);
    }

    bool Install()
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

        Diagnostics::LogWorldspace("Loaded Tamriel test", tamrielWorldspace);
        Diagnostics::LogWorldspace(
            "Loaded Solstheim test",
            solstheimWorldspace);
    }
}
