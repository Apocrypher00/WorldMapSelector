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

        // Adapt MapChooser's function callback to Skyrim's reference-counted
        // callback interface used by the dynamically sized overload.
        class MessageBoxCallback final : public RE::IMessageBoxCallback
        {
        public:
            explicit MessageBoxCallback(ResultCallback callback) : callback_(callback) {}

            void Run(std::uint8_t button) override
            {
                if (callback_) callback_(button);
            }

        private:
            ResultCallback callback_ = nullptr;
        };
    }

    // Open displays one classic Skyrim message box and reports the selected button index through callback.
    bool Open(const char* message, const std::vector<std::string>& buttons, ResultCallback callback)
    {
        RE::BSString messageText{ message };
        RE::BSTArray<RE::BSString> buttonText;
        buttonText.reserve(buttons.size());
        for (const auto& button : buttons) {
            buttonText.emplace_back(button.c_str());
        }

        RE::BSTSmartPointer<RE::IMessageBoxCallback> messageCallback{
            new MessageBoxCallback(callback)
        };

        return RE::MessageBoxMenu::Create(
            messageText,
            messageCallback,
            buttonPressOffset,
            warningType,
            menuDepth,
            buttonText
        );
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
