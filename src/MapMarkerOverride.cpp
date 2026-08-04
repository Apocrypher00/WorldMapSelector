#include "Hooks.h"
#include "MapMarkerOverride.h"
#include "WorldspaceOverride.h"

namespace WMS::MapMarkerOverride
{
    namespace
    {
        // Vanilla's marker helpers call one another synchronously.
        // Keeping this state thread-local, prevents one marker rebuild from affecting another thread.
        thread_local RE::BSTArray<RE::ObjectRefHandle> selectedMapMarkerHandles;
        thread_local bool suppressPlayerMarkerLoop = false;

        // Skyrim's native function to collect map marker handles from a worldspace.
		// Takes the worldspace to collect from, a pointer to the destination array, and a boolean to include hidden markers.
        using CollectMapMarkerHandles_t = void (*)(RE::TESWorldSpace*, RE::BSTArray<RE::ObjectRefHandle>*, bool);
        static REL::Relocation<CollectMapMarkerHandles_t> collectMapMarkerHandles{ REL::ID(20536) };

		// Skyrim's native function to add the player's current location marker to the map.
		// It takes a pointer to the destination array and a pointer to the player's marker position.
        using AddCurrentLocationMarker_t = void (*)(RE::BSTArray<RE::MapMenuMarker>*, RE::NiPoint3*);
        AddCurrentLocationMarker_t originalAddCurrentLocationMarker = nullptr;

        // Skyrim's native function to add a marker from a handle to the map.
		// It takes a pointer to the destination array and a pointer to the marker handle.
        using AddMarkerFromHandle_t = std::uint64_t (*)(RE::BSTArray<RE::MapMenuMarker>**, RE::ObjectRefHandle*);
        AddMarkerFromHandle_t originalAddMarkerFromHandle = nullptr;

		// Detour for vanilla's AddCurrentLocationMarker.
        // Vanilla calls this immediately before iterating the player's currentMapMarkers array.
        // For a remote map, append markers collected from the selected worldspace, and omit the player's location marker.
        void AddCurrentLocationMarkerHook(RE::BSTArray<RE::MapMenuMarker>* mapMarkers, RE::NiPoint3* playerMarkerPosition)
        {
            suppressPlayerMarkerLoop = false;

            auto* selectedWorldspace = WorldspaceOverride::GetSelectedMapWorldspace();

			// If we're not processing a remote map, call the original function to add the player's current location marker.
            if (!selectedWorldspace) {
                originalAddCurrentLocationMarker(mapMarkers, playerMarkerPosition);
                return;
            }

            if (!mapMarkers) return;

			// Collect all map marker handles from the selected worldspace.
			selectedMapMarkerHandles.clear();
            collectMapMarkerHandles(selectedWorldspace, std::addressof(selectedMapMarkerHandles), false);

			// Append the collected handles to the mapMarkers array, using the original AddMarkerFromHandle function.
            const auto markerCountBefore = mapMarkers->size();
            auto* destination = mapMarkers;
            for (auto& handle : selectedMapMarkerHandles) {
                originalAddMarkerFromHandle(std::addressof(destination), std::addressof(handle));
            }

            suppressPlayerMarkerLoop = true;

            SKSE::log::debug(
                "Remote map {:08X}: omitted current location; collected {} handles and added {} markers.",
                selectedWorldspace->GetFormID(), selectedMapMarkerHandles.size(), mapMarkers->size() - markerCountBefore
            );
        }

		// Detour for vanilla's AddMarkerFromHandle.
        // Returning zero from this function tells the caller to stop iterating currentMapMarkers.
        // The selected markers were already appended by AddCurrentLocationMarkerHook.
        std::uint64_t AddMarkerFromHandleHook(RE::BSTArray<RE::MapMenuMarker>** destination, RE::ObjectRefHandle* handle)
        {
            if (suppressPlayerMarkerLoop) {
                suppressPlayerMarkerLoop = false;
                return 0;
            } else {
                return originalAddMarkerFromHandle(destination, handle);
            }
        }
    }

    // Creates the ordinary world-marker detours in a disabled state.
    bool CreateHooks()
    {
        return (
            Hooks::Create("AddCurrentLocationMarker", REL::ID(53076), AddCurrentLocationMarkerHook, originalAddCurrentLocationMarker) &&
            Hooks::Create("AddMarkerFromHandle",      REL::ID(53126), AddMarkerFromHandleHook,      originalAddMarkerFromHandle     )
        );
    }
}
