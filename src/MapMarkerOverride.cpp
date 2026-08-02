#include "MapMarkerOverride.h"
#include "WorldspaceOverride.h"

#include <MinHook.h>

namespace WMS::MapMarkerOverride
{
    namespace
    {
        // Vanilla's marker helpers call one another synchronously. Keeping this
        // state thread-local prevents one marker rebuild from affecting another thread.
        thread_local RE::BSTArray<RE::ObjectRefHandle> selectedMapMarkerHandles;
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

        // Vanilla calls this immediately before iterating the player's currentMapMarkers array.
        // For a remote map, append markers collected from the selected worldspace,
        // and omit the player's location marker.
        void AddCurrentLocationMarkerHook(
            RE::BSTArray<RE::MapMenuMarker>* mapMarkers,
            RE::NiPoint3* playerMarkerPosition)
        {
            suppressPlayerMarkerLoop = false;
            auto* selectedWorldspace = WorldspaceOverride::GetSelectedMapWorldspace();

            if (!selectedWorldspace) {
                originalAddCurrentLocationMarker(mapMarkers, playerMarkerPosition);
                return;
            }

            if (!mapMarkers) { return; }

            static REL::Relocation<CollectMapMarkerHandles_t> collectMapMarkerHandles{
                REL::ID(20536)
            };

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
            SKSE::log::debug(
                "Remote map {:08X}: omitted current location; collected {} "
                "handles and added {} markers.",
                selectedWorldspace->GetFormID(),
                selectedMapMarkerHandles.size(),
                mapMarkers->size() - markerCountBefore
            );
        }

        std::uint64_t AddMarkerFromHandleHook(
            RE::BSTArray<RE::MapMenuMarker>** destination,
            RE::ObjectRefHandle* handle)
        {
            // Returning zero from the first following call tells vanilla's
            // currentMapMarkers loop to stop. The selected markers were
            // already appended by AddCurrentLocationMarkerHook.
            if (suppressPlayerMarkerLoop) {
                suppressPlayerMarkerLoop = false;
                return 0;
            }

            return originalAddMarkerFromHandle(destination, handle);
        }
    }

    bool CreateHooks()
    {
        REL::Relocation<std::uintptr_t> addCurrentLocationMarker{
            REL::ID(53076)
        };
        REL::Relocation<std::uintptr_t> addMarkerFromHandle{
            REL::ID(53126)
        };

        const auto createCurrentLocationStatus = MH_CreateHook(
            reinterpret_cast<void*>(addCurrentLocationMarker.address()),
            reinterpret_cast<void*>(AddCurrentLocationMarkerHook),
            reinterpret_cast<void**>(&originalAddCurrentLocationMarker)
        );
        if (createCurrentLocationStatus != MH_OK) {
            SKSE::log::error(
                "Current-location marker hook creation failed: {}",
                MH_StatusToString(createCurrentLocationStatus));
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
                MH_StatusToString(createMarkerStatus));
            return false;
        }

        SKSE::log::info(
            "Created ordinary MapMenu marker detours at {:X} and {:X}.",
            addCurrentLocationMarker.address(),
            addMarkerFromHandle.address()
        );
        return true;
    }
}
