#include "Hooks.h"
#include "RoutedMarkerOverride.h"
#include "WorldspaceOverride.h"

namespace WMS::RoutedMarkerOverride
{
    namespace
    {
        // True only while a remote world-map call is synchronously processing quest or custom-destination routes on this thread.
        thread_local bool processingSelectedMapRoutes = false;

        using BindCustomDestinationMarker_t = RE::RefHandle* (*)(RE::RefHandle*, RE::BSTArray<RE::MapMenuMarker>*);
        BindCustomDestinationMarker_t originalBindCustomDestinationMarker = nullptr;

        using AppendQuestMarkers_t = void (*)(RE::BSTArray<RE::MapMenuMarker>*, void*, std::uint32_t);
        AppendQuestMarkers_t originalAppendQuestMarkers = nullptr;

        using ResolveRoutedMarkerHandle_t = RE::RefHandle* (*)(RE::RefHandle*, RE::RefHandle*, RE::TeleportPath*, std::uint32_t, bool);
        ResolveRoutedMarkerHandle_t originalResolveRoutedMarkerHandle = nullptr;

        using RouteEntriesShareRootWorldspace_t = bool (*)(RE::TeleportPath*);
        RouteEntriesShareRootWorldspace_t originalRouteEntriesShareRootWorldspace = nullptr;

        // Set the route-processing flag for one synchronous call and restore its previous value automatically when that call finishes.
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
        // Removing entries before the selected worldspace makes that worldspace the route's visible origin for vanilla MapMenu helpers.
        std::optional<RE::TeleportPath> BuildSelectedRouteTail(const RE::TeleportPath* route)
        {
            const auto* selectedWorldspace = WorldspaceOverride::GetSelectedMapWorldspace();
            if (!route || !selectedWorldspace) {
                SKSE::log::trace("Could not trim routed marker: route or selected worldspace was unavailable.");
                return std::nullopt;
            }

            std::optional<std::size_t> selectedIndex;
            for (std::size_t index = 0; index < route->spaces.size(); ++index) {
                const auto& space = route->spaces[index];
                if (space.isWorldspace && space.worldspace == selectedWorldspace) {
                    selectedIndex = index;
                    break;
                }
            }
            if (!selectedIndex) {
                SKSE::log::trace("Rejected routed marker whose path does not enter selected worldspace {:08X}.", selectedWorldspace->GetFormID());
                return std::nullopt;
            }

            RE::TeleportPath trimmed;
            trimmed.start = route->start;
            trimmed.end   = route->end;

            for (std::size_t index = *selectedIndex; index < route->spaces.size(); ++index) {
                trimmed.spaces.push_back(route->spaces[index]);
            }

            for (std::size_t index = *selectedIndex; index < route->teleportRefs.size(); ++index) {
                trimmed.teleportRefs.push_back(route->teleportRefs[index]);
            }

            SKSE::log::trace(
                "Trimmed routed marker for worldspace {:08X}: {} -> {} spaces, {} -> {} teleport references.",
                selectedWorldspace->GetFormID(),
                route->spaces.size(),
                trimmed.spaces.size(),
                route->teleportRefs.size(),
                trimmed.teleportRefs.size()
            );

            return trimmed;
        }

        RE::RefHandle* BindCustomDestinationMarkerHook(RE::RefHandle* result, RE::BSTArray<RE::MapMenuMarker>* mapMarkers)
        {
            if (!WorldspaceOverride::GetSelectedMapWorldspace()) {
                return originalBindCustomDestinationMarker(result, mapMarkers);
            }

            SKSE::log::debug("Processing custom-destination marker routes for the selected map.");
            ScopedSelectedMapRoutes guard;
            return originalBindCustomDestinationMarker(result, mapMarkers);
        }

        void AppendQuestMarkersHook(RE::BSTArray<RE::MapMenuMarker>* mapMarkers, void* objectives, std::uint32_t mapMode)
        {
            // mapMode zero is the world map.
            // Local maps must continue using the player's unmodified routes.
            const bool remoteWorldMap = WorldspaceOverride::GetSelectedMapWorldspace() && mapMode == 0;

            if (!remoteWorldMap) {
                originalAppendQuestMarkers(mapMarkers, objectives, mapMode);
                return;
            }

            SKSE::log::debug("Processing quest-marker routes for the selected world map.");
            ScopedSelectedMapRoutes guard;
            originalAppendQuestMarkers(mapMarkers, objectives, mapMode);
        }

        RE::RefHandle* ResolveRoutedMarkerHandleHook(RE::RefHandle* result, RE::RefHandle* originalHandle, RE::TeleportPath* route, std::uint32_t mapMode, bool validate)
        {
            // These helpers have non-MapMenu callers, so route substitution is allowed only inside one of the scoped calls above.
            if (!processingSelectedMapRoutes) {
                return originalResolveRoutedMarkerHandle(result, originalHandle, route, mapMode, validate);
            }

            // Our replacement logic cannot return a handle if the caller did not provide an output location.
            if (result == nullptr) {
                return nullptr;
            }

            auto trimmed = BuildSelectedRouteTail(route);

            if (!trimmed) {
                // The route never reaches the selected worldspace, so this marker must not be displayed on the selected map.
                // Clear the caller's output handle to indicate that no marker was resolved.
                *result = 0;
                return result;
            }

            return originalResolveRoutedMarkerHandle(result, originalHandle, std::addressof(*trimmed), mapMode, validate);
        }

        bool RouteEntriesShareRootWorldspaceHook(RE::TeleportPath* route)
        {
            if (!processingSelectedMapRoutes) {
                return originalRouteEntriesShareRootWorldspace(route);
            }

            auto trimmed = BuildSelectedRouteTail(route);
            return trimmed && originalRouteEntriesShareRootWorldspace(std::addressof(*trimmed));
        }
    }

    // Creates the quest/custom-destination route detours in a disabled state.
    bool CreateHooks()
    {
        return (
            Hooks::Create("BindCustomDestinationMarker",     REL::RelocationID(52186, 53078), BindCustomDestinationMarkerHook,     originalBindCustomDestinationMarker    ) &&
            Hooks::Create("AppendQuestMarkers",              REL::RelocationID(52181, 53073), AppendQuestMarkersHook,              originalAppendQuestMarkers             ) &&
            Hooks::Create("ResolveRoutedMarkerHandle",       REL::RelocationID(52183, 53075), ResolveRoutedMarkerHandleHook,       originalResolveRoutedMarkerHandle      ) &&
            Hooks::Create("RouteEntriesShareRootWorldspace", REL::RelocationID(52193, 53085), RouteEntriesShareRootWorldspaceHook, originalRouteEntriesShareRootWorldspace)
        );
    }
}
