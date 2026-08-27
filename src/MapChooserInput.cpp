#include "Config.h"
#include "MapChooser.h"
#include "MapChooserInput.h"
#include "MapMenuKeyHint.h"

namespace WMS::MapChooserInput
{
    namespace
    {
        // The sink receives linked lists of input events from Skyrim.
        class InputEventSink final : public RE::BSTEventSink<RE::InputEvent*>
        {
            public:
                RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* events, RE::BSTEventSource<RE::InputEvent*>*) override
                {
                    static std::optional<bool> previousGamepadMode;

                    if (!events) return RE::BSEventNotifyControl::kContinue;

                    const auto configuredKey = Config::GetOpenSelectorKey();

                    // events points to the first pointer in Skyrim's linked list.
                    // Each event->next advances until a null pointer ends the list.
                    for (auto* event = *events; event; event = event->next) {
                        // SkyUI rebuilds its MapMenu button panel when input changes between
                        // keyboard/mouse and controller. Restore our keyboard hint after that rebuild.
                        const bool usesGamepad = event->device == RE::INPUT_DEVICE::kGamepad;
                        const bool usesKeyboardOrMouse =
                            event->device == RE::INPUT_DEVICE::kKeyboard || event->device == RE::INPUT_DEVICE::kMouse;

                        if ((usesGamepad || usesKeyboardOrMouse) &&
                            (!previousGamepadMode || *previousGamepadMode != usesGamepad)) {
                            previousGamepadMode = usesGamepad;
                            MapMenuKeyHint::RefreshAfterInputModeChange();
                        }

                        // AsButtonEvent returns null when this input event is not a button.
                        const auto* button = event->AsButtonEvent();
                        if (!button || !button->IsDown()) {
                            continue;
                        }

                        // LocalMapMenu changes mode later in the same input dispatch. Refresh the
                        // SkyUI hint afterward so it follows the resulting world/local map state.
                        const auto* userEvents = RE::UserEvents::GetSingleton();
                        if (userEvents && button->GetUserEvent() == userEvents->localMap) {
                            MapMenuKeyHint::RefreshAfterMapModeChange();
                        }

                        // Skyrim maps Escape, Tab, and controller Back buttons to its logical Cancel event.
                        const bool isCancel = userEvents && button->GetUserEvent() == userEvents->cancel;

                        if (isCancel && MapChooser::Dismiss()) {
                            return RE::BSEventNotifyControl::kStop;
                        }

                        if (button->device != RE::INPUT_DEVICE::kKeyboard) {
                            continue;
                        }

                        // Process the configured hotkey only when the map chooser is not already open.
                        // MapChooser::Open performs the menu-state checks before displaying anything.
                        if (configuredKey != 0 && button->GetIDCode() == configuredKey) {
                            MapChooser::Open();
                            break;
                        }
                    }

                    return RE::BSEventNotifyControl::kContinue;
                }
        };
    }

    // Register installs the hotkey and chooser-dismiss listeners.
    bool Register()
    {
        auto* input = RE::BSInputDeviceManager::GetSingleton();
        if (!input) {
            SKSE::log::error("Could not get the input-device manager.");
            return false;
        }

        // Static lifetime keeps the registered sink alive for the whole process.
        static InputEventSink inputEventSink;
        // & passes the sink's address to Skyrim rather than copying the object.
        input->AddEventSink(&inputEventSink);

        SKSE::log::info("Registered map chooser hotkey 0x{:02X}.", Config::GetOpenSelectorKey());
        return true;
    }
}
