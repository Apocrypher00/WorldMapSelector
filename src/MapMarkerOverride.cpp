#include "MapMarkerOverride.h"
#include "WorldspaceOverride.h"

#include <MinHook.h>

namespace WMS::MapMarkerOverride
{
    namespace
    {
        thread_local RE::BSTArray<RE::ObjectRefHandle>
            selectedMapMarkerHandles;
        thread_local bool suppressPlayerMarkerLoop = false;

        using CollectMapMarkerHandles_t = void (*)(
            RE::TESWorldSpace*,
            RE::BSTArray<RE::ObjectRefHandle>*,
            bool
        );

        using AddCurrentLocationMarker_t = void (*)(
            RE::BSTArray<RE::MapMenuMarker>*,
            RE::NiPoint3*
        );
        AddCurrentLocationMarker_t originalAddCurrentLocationMarker = nullptr;

        using AddMarkerFromHandle_t = std::uint64_t (*)(
            RE::BSTArray<RE::MapMenuMarker>**,
            RE::ObjectRefHandle*
         );
        AddMarkerFromHandle_t originalAddMarkerFromHandle = nullptr;

        using BindCustomDestinationMarker_t = RE::RefHandle* (*)(
            RE::RefHandle*,
            RE::BSTArray<RE::MapMenuMarker>*
        );
        BindCustomDestinationMarker_t originalBindCustomDestinationMarker =
            nullptr;

        using AppendQuestMarkers_t = void (*)(
            RE::BSTArray<RE::MapMenuMarker>*,
            void*,
            std::uint32_t
        );
        AppendQuestMarkers_t originalAppendQuestMarkers = nullptr;

        void AddCurrentLocationMarkerHook(RE::BSTArray<RE::MapMenuMarker>* mapMarkers, RE::NiPoint3* playerMarkerPosition)
        {
            suppressPlayerMarkerLoop = false;
            auto* selectedWorldspace = WorldspaceOverride::GetSelectedMapWorldspace();

            if (!selectedWorldspace) {
                originalAddCurrentLocationMarker(mapMarkers, playerMarkerPosition);
                return;
            }

            if (!mapMarkers) {
                return;
            }

            static REL::Relocation<CollectMapMarkerHandles_t> collectMapMarkerHandles{ REL::ID(20536) };

            selectedMapMarkerHandles.clear();
            collectMapMarkerHandles(
                selectedWorldspace,
                std::addressof(selectedMapMarkerHandles),
                false
            );

            const auto markerCountBefore = mapMarkers->size();
            auto* destination = mapMarkers;
            for (auto& handle : selectedMapMarkerHandles) {
                originalAddMarkerFromHandle(
                    std::addressof(destination),
                    std::addressof(handle)
                );
            }

            suppressPlayerMarkerLoop = true;
            SKSE::log::info(
                "Remote map {:08X}: omitted current location; collected {} "
                "handles and added {} markers.",
                selectedWorldspace->GetFormID(),
                selectedMapMarkerHandles.size(),
                mapMarkers->size() - markerCountBefore
            );
        }

        std::uint64_t AddMarkerFromHandleHook(RE::BSTArray<RE::MapMenuMarker>** destination, RE::ObjectRefHandle* handle)
        {
            if (suppressPlayerMarkerLoop) {
                suppressPlayerMarkerLoop = false;
                return 0;
            }

            return originalAddMarkerFromHandle(destination, handle);
        }

        RE::RefHandle* BindCustomDestinationMarkerHook(
            RE::RefHandle* result,
            RE::BSTArray<RE::MapMenuMarker>* mapMarkers)
        {
            if (WorldspaceOverride::GetSelectedMapWorldspace() && mapMarkers) {
                bool suppressed = false;

                for (auto& marker : *mapMarkers) {
                    if (marker.type == 2 && marker.door == 0) {
                        marker.ref = 0;
                        suppressed = true;
                    }
                }

                if (suppressed) {
                    if (result) {
                        *result = 0;
                    }
                    return result;
                }
            }

            return originalBindCustomDestinationMarker(result, mapMarkers);
        }

        void AppendQuestMarkersHook(
            RE::BSTArray<RE::MapMenuMarker>* mapMarkers,
            void* objectives,
            std::uint32_t mapMode)
        {
            const bool remoteWorldMap =
                WorldspaceOverride::GetSelectedMapWorldspace() &&
                mapMode == 0;

            if (remoteWorldMap) {
                return;
            }

            originalAppendQuestMarkers(mapMarkers, objectives, mapMode);
        }
    }

    bool Install()
    {
        REL::Relocation<std::uintptr_t> addCurrentLocationMarker{
            REL::ID(53076)
        };
        REL::Relocation<std::uintptr_t> addMarkerFromHandle{
            REL::ID(53126)
        };
        REL::Relocation<std::uintptr_t> bindCustomDestinationMarker{
            REL::ID(53078)
        };
        REL::Relocation<std::uintptr_t> appendQuestMarkers{
            REL::ID(53073)
        };

        const auto createCurrentLocationStatus = MH_CreateHook(
            reinterpret_cast<void*>(addCurrentLocationMarker.address()),
            reinterpret_cast<void*>(AddCurrentLocationMarkerHook),
            reinterpret_cast<void**>(&originalAddCurrentLocationMarker)
        );
        if (createCurrentLocationStatus != MH_OK) {
            SKSE::log::error(
                "Current-location marker hook creation failed: {}",
                static_cast<int>(createCurrentLocationStatus));
            return false;
        }

        const auto createMarkerStatus = MH_CreateHook(
            reinterpret_cast<void*>(addMarkerFromHandle.address()),
            reinterpret_cast<void*>(AddMarkerFromHandleHook),
            reinterpret_cast<void**>(&originalAddMarkerFromHandle)
        );
        if (createMarkerStatus != MH_OK) {
            SKSE::log::error(
                "Marker-from-handle hook creation failed: {}",
                static_cast<int>(createMarkerStatus));
            return false;
        }

        const auto createCustomDestinationStatus = MH_CreateHook(
            reinterpret_cast<void*>(bindCustomDestinationMarker.address()),
            reinterpret_cast<void*>(BindCustomDestinationMarkerHook),
            reinterpret_cast<void**>(&originalBindCustomDestinationMarker)
        );
        if (createCustomDestinationStatus != MH_OK) {
            SKSE::log::error(
                "Custom-destination marker hook creation failed: {}",
                static_cast<int>(createCustomDestinationStatus)
            );
            return false;
        }

        const auto createQuestMarkersStatus = MH_CreateHook(
            reinterpret_cast<void*>(appendQuestMarkers.address()),
            reinterpret_cast<void*>(AppendQuestMarkersHook),
            reinterpret_cast<void**>(&originalAppendQuestMarkers)
        );
        if (createQuestMarkersStatus != MH_OK) {
            SKSE::log::error(
                "Quest-marker hook creation failed: {}",
                static_cast<int>(createQuestMarkersStatus)
            );
            return false;
        }

        const auto enableCurrentLocationStatus = MH_EnableHook(
            reinterpret_cast<void*>(addCurrentLocationMarker.address())
        );
        if (enableCurrentLocationStatus != MH_OK) {
            SKSE::log::error(
                "Current-location marker hook activation failed: {}",
                static_cast<int>(enableCurrentLocationStatus));
            return false;
        }

        const auto enableMarkerStatus = MH_EnableHook(
            reinterpret_cast<void*>(addMarkerFromHandle.address())
        );
        if (enableMarkerStatus != MH_OK) {
            SKSE::log::error(
                "Marker-from-handle hook activation failed: {}",
                static_cast<int>(enableMarkerStatus)
            );
            return false;
        }

        const auto enableCustomDestinationStatus = MH_EnableHook(
            reinterpret_cast<void*>(bindCustomDestinationMarker.address())
        );
        if (enableCustomDestinationStatus != MH_OK) {
            SKSE::log::error(
                "Custom-destination marker hook activation failed: {}",
                static_cast<int>(enableCustomDestinationStatus)
            );
            return false;
        }

        const auto enableQuestMarkersStatus = MH_EnableHook(
            reinterpret_cast<void*>(appendQuestMarkers.address())
        );
        if (enableQuestMarkersStatus != MH_OK) {
            SKSE::log::error(
                "Quest-marker hook activation failed: {}",
                static_cast<int>(enableQuestMarkersStatus)
            );
            return false;
        }

        SKSE::log::info(
            "Installed MapMenu marker detours at {:X}, {:X}, {:X}, and {:X}.",
            addCurrentLocationMarker.address(),
            addMarkerFromHandle.address(),
            bindCustomDestinationMarker.address(),
            appendQuestMarkers.address()
        );
        return true;
    }
}
