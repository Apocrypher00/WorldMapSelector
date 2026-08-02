#include "RoutedMarkerOverride.h"
#include "WorldspaceOverride.h"

#include <MinHook.h>

namespace WMS::RoutedMarkerOverride
{
    namespace
    {
        // True only while a remote world-map call is synchronously processing
        // quest or custom-destination routes on this thread.
        thread_local bool processingSelectedMapRoutes = false;

        using BindCustomDestinationMarker_t = RE::RefHandle* (*)(
            RE::RefHandle*,
            RE::BSTArray<RE::MapMenuMarker>*
        );
        BindCustomDestinationMarker_t originalBindCustomDestinationMarker = nullptr;

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
        ResolveRoutedMarkerHandle_t originalResolveRoutedMarkerHandle = nullptr;

        using RouteEntriesShareRootWorldspace_t = bool (*)(RE::TeleportPath*);
        RouteEntriesShareRootWorldspace_t originalRouteEntriesShareRootWorldspace = nullptr;

        // Set the route-processing flag for one synchronous call and restore
        // its previous value automatically when that call finishes.
        class ScopedSelectedMapRoutes
        {
        public:
            ScopedSelectedMapRoutes() :
                previousValue(processingSelectedMapRoutes)
            {
                processingSelectedMapRoutes = true;
            }

            ~ScopedSelectedMapRoutes()
            {
                processingSelectedMapRoutes = previousValue;
            }

            ScopedSelectedMapRoutes(const ScopedSelectedMapRoutes&) = delete;
            ScopedSelectedMapRoutes& operator=(const ScopedSelectedMapRoutes&) = delete;

        private:
            bool previousValue;
        };

        // Routes begin at the player and cross doors/worldspaces toward a marker.
        // Removing entries before the selected worldspace makes that worldspace
        // the route's visible origin for vanilla MapMenu helpers.
        std::optional<RE::TeleportPath> BuildSelectedRouteTail(const RE::TeleportPath* route)
        {
            const auto* selectedWorldspace = WorldspaceOverride::GetSelectedMapWorldspace();
            if (!route || !selectedWorldspace) { return std::nullopt; }

            std::optional<std::size_t> selectedIndex;
            for (std::size_t index = 0; index < route->spaces.size(); ++index) {
                const auto& space = route->spaces[index];
                if (space.isWorldspace && space.worldspace == selectedWorldspace) {
                    selectedIndex = index;
                    break;
                }
            }
            if (!selectedIndex) { return std::nullopt; }

            RE::TeleportPath trimmed;
            trimmed.start = route->start;
            trimmed.end   = route->end;

            for (std::size_t index = *selectedIndex; index < route->spaces.size(); ++index) {
                trimmed.spaces.push_back(route->spaces[index]);
            }

            for (std::size_t index = *selectedIndex; index < route->teleportRefs.size(); ++index) {
                trimmed.teleportRefs.push_back(route->teleportRefs[index]);
            }

            return trimmed;
        }

        RE::RefHandle* BindCustomDestinationMarkerHook(
            RE::RefHandle* result,
            RE::BSTArray<RE::MapMenuMarker>* mapMarkers)
        {
            if (!WorldspaceOverride::GetSelectedMapWorldspace()) {
                return originalBindCustomDestinationMarker(result, mapMarkers);
            }

            ScopedSelectedMapRoutes guard;
            return originalBindCustomDestinationMarker(result, mapMarkers);
        }

        void AppendQuestMarkersHook(
            RE::BSTArray<RE::MapMenuMarker>* mapMarkers,
            void* objectives,
            std::uint32_t mapMode)
        {
            // mapMode zero is the world map. Local maps must continue using
            // the player's unmodified routes.
            const bool remoteWorldMap =
                WorldspaceOverride::GetSelectedMapWorldspace() &&
                mapMode == 0;

            if (!remoteWorldMap) {
                originalAppendQuestMarkers(mapMarkers, objectives, mapMode);
                return;
            }

            ScopedSelectedMapRoutes guard;
            originalAppendQuestMarkers(mapMarkers, objectives, mapMode);
        }

        RE::RefHandle* ResolveRoutedMarkerHandleHook(
            RE::RefHandle* result,
            RE::RefHandle* originalHandle,
            RE::TeleportPath* route,
            std::uint32_t mapMode,
            bool validate)
        {
            // These helpers have non-MapMenu callers, so route substitution is
            // allowed only inside one of the scoped calls above.
            if (!processingSelectedMapRoutes) {
                return originalResolveRoutedMarkerHandle(
                    result,
                    originalHandle,
                    route,
                    mapMode,
                    validate);
            }

            auto trimmed = BuildSelectedRouteTail(route);
            if (!trimmed) {
                if (result) { *result = 0; }
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
            if (!processingSelectedMapRoutes) {
                return originalRouteEntriesShareRootWorldspace(route);
            }

            auto trimmed = BuildSelectedRouteTail(route);
            return trimmed &&
                   originalRouteEntriesShareRootWorldspace(std::addressof(*trimmed));
        }
    }

    bool CreateHooks()
    {
        REL::Relocation<std::uintptr_t> bindCustomDestinationMarker{ REL::ID(53078) };
        REL::Relocation<std::uintptr_t> appendQuestMarkers{ REL::ID(53073) };
        REL::Relocation<std::uintptr_t> resolveRoutedMarkerHandle{ REL::ID(53075) };
        REL::Relocation<std::uintptr_t> routeEntriesShareRootWorldspace{ REL::ID(53085) };

        const auto createCustomDestinationStatus = MH_CreateHook(
            reinterpret_cast<void*>(bindCustomDestinationMarker.address()),
            reinterpret_cast<void*>(BindCustomDestinationMarkerHook),
            reinterpret_cast<void**>(&originalBindCustomDestinationMarker)
        );
        if (createCustomDestinationStatus != MH_OK) {
            SKSE::log::error(
                "Custom-destination marker hook creation failed: {}",
                static_cast<int>(createCustomDestinationStatus));
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
                static_cast<int>(createQuestMarkersStatus));
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
                static_cast<int>(createResolveRoutedMarkerStatus));
            return false;
        }

        const auto createRouteRootStatus = MH_CreateHook(
            reinterpret_cast<void*>(routeEntriesShareRootWorldspace.address()),
            reinterpret_cast<void*>(RouteEntriesShareRootWorldspaceHook),
            reinterpret_cast<void**>(&originalRouteEntriesShareRootWorldspace)
        );
        if (createRouteRootStatus != MH_OK) {
            SKSE::log::error(
                "Route-root comparison hook creation failed: {}",
                static_cast<int>(createRouteRootStatus));
            return false;
        }

        SKSE::log::info(
            "Created routed MapMenu marker detours at {:X}, {:X}, {:X}, and {:X}.",
            bindCustomDestinationMarker.address(),
            appendQuestMarkers.address(),
            resolveRoutedMarkerHandle.address(),
            routeEntriesShareRootWorldspace.address()
        );
        return true;
    }
}
