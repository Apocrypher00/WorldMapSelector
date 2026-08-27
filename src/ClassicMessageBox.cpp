#include "ClassicMessageBox.h"

namespace WMS::ClassicMessageBox
{
    namespace
    {
        // Keep callback results equal to their zero-based indexes in buttons.
        constexpr std::uint8_t buttonPressOffset = 0;

        // CommonLib exposes warning type as an undocumented numeric identifier.
        constexpr std::int32_t warningType = 0;

        // CommonLib documents MessageBoxMenu as using menu depth 10.
        constexpr std::int32_t menuDepth = 10;
    }

    // Open displays one classic Skyrim message box and reports the selected button index through callback.
    bool Open(const char* message, const std::vector<std::string>& buttons, ResultCallback callback)
    {
        // Convert an owned string to the C-style string expected by Skyrim.
        // buttons keeps every string alive until MessageBoxMenu::Create returns.
        const auto button = [&](std::size_t index) {
            return buttons[index].c_str();
        };

        // Skyrim's Create function is variadic, so C++ must make a separate call for every supported button count.
        switch (buttons.size()) {
            case 1:
                return RE::MessageBoxMenu::Create(
                    message, callback, buttonPressOffset, warningType, menuDepth,
                    button(0)
                );
            case 2:
                return RE::MessageBoxMenu::Create(
                    message, callback, buttonPressOffset, warningType, menuDepth,
                    button(0), button(1)
                );
            case 3:
                return RE::MessageBoxMenu::Create(
                    message, callback, buttonPressOffset, warningType, menuDepth,
                    button(0), button(1), button(2)
                );
            case 4:
                return RE::MessageBoxMenu::Create(
                    message, callback, buttonPressOffset, warningType, menuDepth,
                    button(0), button(1), button(2), button(3)
                );
            case 5:
                return RE::MessageBoxMenu::Create(
                    message, callback, buttonPressOffset, warningType, menuDepth,
                    button(0), button(1), button(2), button(3), button(4)
                );
            case 6:
                return RE::MessageBoxMenu::Create(
                    message, callback, buttonPressOffset, warningType, menuDepth,
                    button(0), button(1), button(2), button(3), button(4), button(5)
                );
            case 7:
                return RE::MessageBoxMenu::Create(
                    message, callback, buttonPressOffset, warningType, menuDepth,
                    button(0), button(1), button(2), button(3), button(4), button(5), button(6)
                );
            case 8:
                return RE::MessageBoxMenu::Create(
                    message, callback, buttonPressOffset, warningType, menuDepth,
                    button(0), button(1), button(2), button(3), button(4), button(5), button(6), button(7)
                );
            case 9:
                return RE::MessageBoxMenu::Create(
                    message, callback, buttonPressOffset, warningType, menuDepth,
                    button(0), button(1), button(2), button(3), button(4), button(5), button(6), button(7), button(8)
                );
            case 10:
                return RE::MessageBoxMenu::Create(
                    message, callback, buttonPressOffset, warningType, menuDepth,
                    button(0), button(1), button(2), button(3), button(4), button(5), button(6), button(7), button(8), button(9)
                );
            default:
                return false;
        }
    }

    // Dismiss removes the current message box without reporting any button as selected.
    bool Dismiss()
    {
        auto* data = RE::MessageBoxMenu::GetCurrentMessageBoxData();
        if (!data) return false;

        // Removing the queued data destroys its callback without invoking it.
        RE::MessageBoxMenu::RemoveMessageFromQueue(data);

        // SelectOption normally performs this step after removing its message.
        // Since no Scaleform button was pressed, close the empty menu explicitly.
        if (!RE::MessageBoxMenu::GetCurrentMessageBoxData()) {
            if (auto* queue = RE::UIMessageQueue::GetSingleton()) {
                queue->AddMessage(RE::MessageBoxMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr);
            }
        }

        return true;
    }
}
