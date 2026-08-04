#pragma once

#include <MinHook.h>

namespace WMS::Hooks
{
    // This template is implemented in the header so each call site can compile a correctly typed version for its particular Address Library ID and function pointer.
    template <class AddressID, class Function>
    bool Create(std::string_view name, const AddressID& id, Function detour, Function& original)
    {
        // AddressID accepts either REL::ID or REL::RelocationID.
        // Constructing the relocation resolves that identifier to this runtime's address.
        REL::Relocation<std::uintptr_t> target{ id };

        // MinHook accepts untyped addresses.
        // The final argument points to our function-pointer variable, where MinHook stores the callable original trampoline.
        const auto status = MH_CreateHook(
            reinterpret_cast<void*>(target.address()),
            reinterpret_cast<void*>(detour),
            reinterpret_cast<void**>(std::addressof(original))
        );

        if (status == MH_OK) {
            SKSE::log::debug("Created {} hook at {:X}.", name, target.address());
            return true;
        } else {
            SKSE::log::error("{} hook creation failed at {:X}: {}", name, target.address(), MH_StatusToString(status));
            return false;
        }
    }

    bool Initialize();
    bool EnableAll();
    void Reset();
}
