#include "Config.h"
#include "WorldspaceCatalog.h"
#include "WorldspaceOverride.h"

#include <MinHook.h>
#include <intrin.h>

namespace WMS::WorldspaceOverride
{
    namespace
    {
        std::atomic<RE::TESWorldSpace*> selectedMapWorldspace = nullptr;

        std::mutex sessionLock;
        bool sessionActive = false;

        std::mutex selectionLogLock;
        std::string lastSelectionLog;

        using ResolveMapWorldSpace_t = RE::TESWorldSpace* (*)();
        ResolveMapWorldSpace_t originalResolveMapWorldSpace = nullptr;

        const char* DescribeResolverCaller(std::uintptr_t callRva)
        {
            switch (callRva) {
            case 0x983B40:
                return "MapMenu constructor";
            case 0x98524E:
                return "MapMenu::ProcessMessage";
            case 0x9885BD:
                return "world-map resource initialization";
            case 0x988955:
                return "world-map lifecycle setup";
            case 0x9892EA:
                return "world-map lifecycle A";
            case 0x989452:
                return "world-map lifecycle B";
            default:
                return "unknown";
            }
        }

        void LogResolverCall(
            std::uintptr_t returnAddress,
            RE::TESWorldSpace* actualWorldspace)
        {
            constexpr std::uintptr_t callInstructionSize = 5;

            const auto returnRva =
                returnAddress - REL::Module::get().base();
            const auto callRva =
                returnRva >= callInstructionSize
                    ? returnRva - callInstructionSize
                    : returnRva;
            const auto* latchedWorldspace =
                selectedMapWorldspace.load(
                    std::memory_order_acquire);
            auto* ui = RE::UI::GetSingleton();
            const auto mapMenuOpen =
                ui && ui->IsMenuOpen(RE::MapMenu::MENU_NAME);

            SKSE::log::info(
                "Resolver call: caller=\"{}\", RVA={:X}, "
                "MapMenuOpen={}, actual={:08X}, "
                "sessionActive={}, selected={:08X}.",
                DescribeResolverCaller(callRva),
                callRva,
                mapMenuOpen,
                actualWorldspace
                    ? actualWorldspace->GetFormID()
                    : 0,
                sessionActive,
                latchedWorldspace
                    ? latchedWorldspace->GetFormID()
                    : 0);
        }

        bool IsNewSelectionLog(std::string_view message)
        {
            std::scoped_lock lock(selectionLogLock);
            if (lastSelectionLog == message) {
                return false;
            }

            lastSelectionLog = message;
            return true;
        }

        RE::TESWorldSpace* ResolveMapWorldSpaceHook()
        {
            const auto returnAddress =
                reinterpret_cast<std::uintptr_t>(_ReturnAddress());
            auto* actualWorldspace = originalResolveMapWorldSpace();

            std::scoped_lock sessionGuard(sessionLock);

            LogResolverCall(returnAddress, actualWorldspace);

            if (sessionActive) {
                if (auto* selectedWorldspace =
                        selectedMapWorldspace.load(
                            std::memory_order_acquire)) {
                    return selectedWorldspace;
                }

                return actualWorldspace;
            }

            const auto requestedIdentifier =
                Config::ReadMapSelection();
            const auto selection =
                WorldspaceCatalog::ResolveSelection(requestedIdentifier);

            if (!selection.error.empty()) {
                selectedMapWorldspace.store(
                    nullptr,
                    std::memory_order_release);

                if (IsNewSelectionLog(selection.error)) {
                    SKSE::log::warn(
                        "{} Using Default.",
                        selection.error);
                }
                return actualWorldspace;
            }

            auto* selectedWorldspace = selection.worldspace;
            if (selection.isDefault ||
                selectedWorldspace == actualWorldspace) {
                selectedMapWorldspace.store(
                    nullptr,
                    std::memory_order_release);

                const auto description =
                    selection.isDefault
                        ? std::string("Default")
                        : fmt::format(
                              "Default (requested map {:08X} is current)",
                              selectedWorldspace->GetFormID());

                if (IsNewSelectionLog(description)) {
                    SKSE::log::info(
                        "Map selection: {}.",
                        description);
                }
                return actualWorldspace;
            }

            selectedMapWorldspace.store(
                selectedWorldspace,
                std::memory_order_release);

            const auto description = fmt::format(
                "{} -> {:08X}",
                requestedIdentifier,
                selectedWorldspace->GetFormID());
            if (IsNewSelectionLog(description)) {
                SKSE::log::info(
                    "Map selection: {}.",
                    description);
            }

            return selectedWorldspace;
        }
    }

    RE::TESWorldSpace* GetSelectedMapWorldspace()
    {
        return selectedMapWorldspace.load(std::memory_order_acquire);
    }

    void BeginSession()
    {
        std::scoped_lock sessionGuard(sessionLock);

        sessionActive = true;
        const auto* selectedWorldspace =
            selectedMapWorldspace.load(
                std::memory_order_acquire);

        SKSE::log::info(
            "Froze map selection for open session: {:08X}.",
            selectedWorldspace
                ? selectedWorldspace->GetFormID()
                : 0);
    }

    void ResetSession()
    {
        std::scoped_lock sessionGuard(sessionLock);

        const auto wasActive = sessionActive;
        sessionActive = false;
        selectedMapWorldspace.store(
            nullptr,
            std::memory_order_release);

        if (wasActive) {
            SKSE::log::info(
                "Cleared map selection session state.");
        }
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
}
