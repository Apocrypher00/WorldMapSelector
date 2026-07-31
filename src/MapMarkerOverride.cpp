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
        thread_local bool processingRemoteQuestRoutes = false;

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

        using ResolveRoutedMarkerHandle_t = RE::RefHandle* (*)(
            RE::RefHandle*,
            RE::RefHandle*,
            RE::TeleportPath*,
            std::uint32_t,
            bool
        );
        ResolveRoutedMarkerHandle_t originalResolveRoutedMarkerHandle =
            nullptr;

        using RouteEntriesShareRootWorldspace_t = bool (*)(
            RE::TeleportPath*
        );
        RouteEntriesShareRootWorldspace_t
            originalRouteEntriesShareRootWorldspace = nullptr;

        class ScopedRemoteQuestRoutes
        {
        public:
            ScopedRemoteQuestRoutes() :
                previousValue(processingRemoteQuestRoutes)
            {
                processingRemoteQuestRoutes = true;
            }

            ~ScopedRemoteQuestRoutes()
            {
                processingRemoteQuestRoutes = previousValue;
            }

            ScopedRemoteQuestRoutes(const ScopedRemoteQuestRoutes&) = delete;
            ScopedRemoteQuestRoutes& operator=(
                const ScopedRemoteQuestRoutes&) = delete;

        private:
            bool previousValue;
        };

        std::optional<RE::TeleportPath> BuildSelectedRouteTail(
            const RE::TeleportPath* route)
        {
            const auto* selectedWorldspace =
                WorldspaceOverride::GetSelectedMapWorldspace();
            if (!route || !selectedWorldspace) {
                return std::nullopt;
            }

            std::optional<std::size_t> selectedIndex;
            for (std::size_t index = 0; index < route->spaces.size(); ++index) {
                const auto& space = route->spaces[index];
                if (space.isWorldspace &&
                    space.worldspace == selectedWorldspace) {
                    selectedIndex = index;
                    break;
                }
            }

            if (!selectedIndex) {
                return std::nullopt;
            }

            RE::TeleportPath trimmed;
            trimmed.start = route->start;
            trimmed.end = route->end;

            for (std::size_t index = *selectedIndex;
                 index < route->spaces.size();
                 ++index) {
                trimmed.spaces.push_back(route->spaces[index]);
            }

            for (std::size_t index = *selectedIndex;
                 index < route->teleportRefs.size();
                 ++index) {
                trimmed.teleportRefs.push_back(route->teleportRefs[index]);
            }

            return trimmed;
        }

        using HandleCustomDestinationClick_t = void (*)(RE::MapMenu*);
        HandleCustomDestinationClick_t originalHandleCustomDestinationClick =
            nullptr;

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

            if (!remoteWorldMap) {
                originalAppendQuestMarkers(mapMarkers, objectives, mapMode);
                return;
            }

            ScopedRemoteQuestRoutes guard;
            originalAppendQuestMarkers(mapMarkers, objectives, mapMode);
        }

        RE::RefHandle* ResolveRoutedMarkerHandleHook(
            RE::RefHandle* result,
            RE::RefHandle* originalHandle,
            RE::TeleportPath* route,
            std::uint32_t mapMode,
            bool validate)
        {
            if (!processingRemoteQuestRoutes) {
                return originalResolveRoutedMarkerHandle(
                    result,
                    originalHandle,
                    route,
                    mapMode,
                    validate);
            }

            auto trimmed = BuildSelectedRouteTail(route);
            if (!trimmed) {
                if (result) {
                    *result = 0;
                }
                return result;
            }

            return originalResolveRoutedMarkerHandle(
                result,
                originalHandle,
                std::addressof(*trimmed),
                mapMode,
                validate);
        }

        bool RouteEntriesShareRootWorldspaceHook(RE::TeleportPath* route)
        {
            if (!processingRemoteQuestRoutes) {
                return originalRouteEntriesShareRootWorldspace(route);
            }

            auto trimmed = BuildSelectedRouteTail(route);
            return trimmed &&
                   originalRouteEntriesShareRootWorldspace(
                       std::addressof(*trimmed));
        }

        void HandleCustomDestinationClickHook(RE::MapMenu* mapMenu)
        {
            if (WorldspaceOverride::GetSelectedMapWorldspace()) {
                return;
            }

            originalHandleCustomDestinationClick(mapMenu);
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
        REL::Relocation<std::uintptr_t> resolveRoutedMarkerHandle{
            REL::ID(53075)
        };
        REL::Relocation<std::uintptr_t> routeEntriesShareRootWorldspace{
            REL::ID(53085)
        };
        REL::Relocation<std::uintptr_t> handleCustomDestinationClick{
            REL::ID(53113)
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

        const auto createResolveRoutedMarkerStatus = MH_CreateHook(
            reinterpret_cast<void*>(resolveRoutedMarkerHandle.address()),
            reinterpret_cast<void*>(ResolveRoutedMarkerHandleHook),
            reinterpret_cast<void**>(&originalResolveRoutedMarkerHandle)
        );
        if (createResolveRoutedMarkerStatus != MH_OK) {
            SKSE::log::error(
                "Routed-marker resolver hook creation failed: {}",
                static_cast<int>(createResolveRoutedMarkerStatus)
            );
            return false;
        }

        const auto createRouteRootStatus = MH_CreateHook(
            reinterpret_cast<void*>(
                routeEntriesShareRootWorldspace.address()),
            reinterpret_cast<void*>(RouteEntriesShareRootWorldspaceHook),
            reinterpret_cast<void**>(
                &originalRouteEntriesShareRootWorldspace)
        );
        if (createRouteRootStatus != MH_OK) {
            SKSE::log::error(
                "Route-root comparison hook creation failed: {}",
                static_cast<int>(createRouteRootStatus)
            );
            return false;
        }

        const auto createCustomDestinationClickStatus = MH_CreateHook(
            reinterpret_cast<void*>(handleCustomDestinationClick.address()),
            reinterpret_cast<void*>(HandleCustomDestinationClickHook),
            reinterpret_cast<void**>(&originalHandleCustomDestinationClick)
        );
        if (createCustomDestinationClickStatus != MH_OK) {
            SKSE::log::error(
                "Custom-destination click hook creation failed: {}",
                static_cast<int>(createCustomDestinationClickStatus)
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

        const auto enableResolveRoutedMarkerStatus = MH_EnableHook(
            reinterpret_cast<void*>(resolveRoutedMarkerHandle.address())
        );
        if (enableResolveRoutedMarkerStatus != MH_OK) {
            SKSE::log::error(
                "Routed-marker resolver hook activation failed: {}",
                static_cast<int>(enableResolveRoutedMarkerStatus)
            );
            return false;
        }

        const auto enableRouteRootStatus = MH_EnableHook(
            reinterpret_cast<void*>(
                routeEntriesShareRootWorldspace.address())
        );
        if (enableRouteRootStatus != MH_OK) {
            SKSE::log::error(
                "Route-root comparison hook activation failed: {}",
                static_cast<int>(enableRouteRootStatus)
            );
            return false;
        }

        const auto enableCustomDestinationClickStatus = MH_EnableHook(
            reinterpret_cast<void*>(handleCustomDestinationClick.address())
        );
        if (enableCustomDestinationClickStatus != MH_OK) {
            SKSE::log::error(
                "Custom-destination click hook activation failed: {}",
                static_cast<int>(enableCustomDestinationClickStatus)
            );
            return false;
        }

        SKSE::log::info(
            "Installed MapMenu marker detours at {:X}, {:X}, {:X}, {:X}, "
            "{:X}, {:X}, and {:X}.",
            addCurrentLocationMarker.address(),
            addMarkerFromHandle.address(),
            bindCustomDestinationMarker.address(),
            appendQuestMarkers.address(),
            resolveRoutedMarkerHandle.address(),
            routeEntriesShareRootWorldspace.address(),
            handleCustomDestinationClick.address()
        );
        return true;
    }
}
