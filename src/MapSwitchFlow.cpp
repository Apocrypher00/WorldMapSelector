#include "MapSwitchFlow.h"

namespace WMS::MapSwitchFlow
{
    namespace
    {
        enum class FlowMode
        {
            kNone,
            kOpenAfterChoice,
            kSwitchOpenMap
        };

        std::mutex flowLock;
        FlowMode flowMode = FlowMode::kNone;
        bool reopenAfterMapClose = false;

        void Configure(FlowMode mode)
        {
            std::scoped_lock lock(flowLock);
            flowMode = mode;
        }

        FlowMode Consume()
        {
            std::scoped_lock lock(flowLock);
            const auto result = flowMode;
            flowMode = FlowMode::kNone;
            return result;
        }

        bool QueueSelectedMapOpen()
        {
            // UI messages are queued onto Skyrim's task thread instead of
            // changing menus directly from an input or message-box callback.
            auto* tasks = SKSE::GetTaskInterface();
            if (!tasks) {
                return false;
            }

            SKSE::log::debug("Queued MapMenu to open after map selection.");
            tasks->AddTask([] {
                SKSE::log::trace("Sending MapMenu show message.");
                if (auto* queue =
                        RE::UIMessageQueue::GetSingleton()) {
                    queue->AddMessage(
                        RE::MapMenu::MENU_NAME,
                        RE::UI_MESSAGE_TYPE::kShow,
                        nullptr);
                } else {
                    SKSE::log::error(
                        "Could not get the UI message queue to open MapMenu.");
                }
            });
            return true;
        }
    }

    void ConfigureOutsideMap(bool openAfterChoice)
    {
        SKSE::log::debug(
            "Configured chooser outside MapMenu: openAfterChoice={}.",
            openAfterChoice);
        Configure(
            openAfterChoice
                ? FlowMode::kOpenAfterChoice
                : FlowMode::kNone
        );
    }

    void ConfigureInsideMap()
    {
        SKSE::log::debug(
            "Configured chooser to replace the open MapMenu after selection.");
        Configure(FlowMode::kSwitchOpenMap);
    }

    void Cancel()
    {
        SKSE::log::debug("Cancelled pending map chooser flow.");
        Consume();
    }

    void ApplyAfterChoice()
    {
        switch (Consume()) {
        case FlowMode::kOpenAfterChoice:
            SKSE::log::debug(
                "Map choice completed; opening MapMenu.");
            if (!QueueSelectedMapOpen()) {
                SKSE::log::error(
                    "Could not queue the selected world map to open.");
            }
            break;

        case FlowMode::kSwitchOpenMap:
            SKSE::log::debug(
                "Map choice completed; closing MapMenu before reopening it.");
            {
                std::scoped_lock lock(flowLock);
                reopenAfterMapClose = true;
            }

            if (auto* queue =
                    RE::UIMessageQueue::GetSingleton()) {
                queue->AddMessage(
                    RE::MapMenu::MENU_NAME,
                    RE::UI_MESSAGE_TYPE::kHide,
                    nullptr);
            } else {
                std::scoped_lock lock(flowLock);
                reopenAfterMapClose = false;
            }
            break;

        case FlowMode::kNone:
            SKSE::log::trace(
                "Map choice completed with no pending menu action.");
            break;
        }
    }

    bool OnMapMenuClosed()
    {
        {
            std::scoped_lock lock(flowLock);
            if (!reopenAfterMapClose) {
                return false;
            }
            reopenAfterMapClose = false;
        }

        SKSE::log::debug(
            "MapMenu closed during a map switch; scheduling its replacement.");
        if (QueueSelectedMapOpen()) {
            return true;
        }

        SKSE::log::error("Could not reopen MapMenu after switching maps.");
        return false;
    }
}
