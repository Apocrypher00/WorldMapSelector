#include "MapSwitchFlow.h"

namespace WMS::MapSwitchFlow
{
    namespace
    {
		// Options for what should happen after a map is chosen.
        enum class FlowMode
        {
            kNone,
            kOpenAfterChoice,
            kSwitchOpenMap
        };

		// Lock to protect flow state across threads.
        std::mutex flowLock;

		// The current flow mode for the next map selection.
        FlowMode flowMode = FlowMode::kNone;

		// Whether the MapMenu should be reopened after it is closed.
        bool reopenAfterMapClose = false;

		// Set the flow mode for the next map selection.
        void Configure(FlowMode mode)
        {
            std::scoped_lock lock(flowLock);
            flowMode = mode;
        }

		// Consume and reset the current flow mode.
        FlowMode Consume()
        {
            std::scoped_lock lock(flowLock);
            const auto result = flowMode;
            flowMode = FlowMode::kNone;
            return result;
        }

        // Run on Skyrim's task thread after QueueSelectedMapOpen schedules it.
        void OpenSelectedMapTask()
        {
            SKSE::log::trace("Sending MapMenu show message.");
            if (auto* queue = RE::UIMessageQueue::GetSingleton()) {
                queue->AddMessage(RE::MapMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kShow, nullptr);
            } else {
                SKSE::log::error("Could not get the UI message queue to open MapMenu.");
            }
        }

        // UI messages are queued onto Skyrim's task thread instead of changing menus directly from an input or message-box callback.
        bool QueueSelectedMapOpen()
        {
            auto* tasks = SKSE::GetTaskInterface();
            if (!tasks) return false;

            tasks->AddTask(OpenSelectedMapTask);

            SKSE::log::debug("Queued MapMenu to open after map selection.");
            return true;
        }
    }

    // Records whether a completed choice should open the map.
    void ConfigureOutsideMap(bool openAfterChoice)
    {
        SKSE::log::debug("Configured chooser outside MapMenu: openAfterChoice={}.", openAfterChoice);
        Configure(openAfterChoice ? FlowMode::kOpenAfterChoice : FlowMode::kNone);
    }

    // Records that the existing map must be replaced.
    void ConfigureInsideMap()
    {
        SKSE::log::debug("Configured chooser to replace the open MapMenu after selection.");
        Configure(FlowMode::kSwitchOpenMap);
    }

    // Discards the pending behavior.
    void Cancel()
    {
        SKSE::log::debug("Cancelled pending map chooser flow.");
        Consume();
    }

    // Performs it after a map is selected.
    void ApplyAfterChoice()
    {
        switch (Consume()) {
            case FlowMode::kOpenAfterChoice:
                SKSE::log::debug("Map choice completed; opening MapMenu.");
                if (!QueueSelectedMapOpen()) {
                    SKSE::log::error("Could not queue the selected world map to open.");
                }
                break;

            case FlowMode::kSwitchOpenMap:
                SKSE::log::debug("Map choice completed; closing MapMenu before reopening it.");
                {
                    std::scoped_lock lock(flowLock);
                    reopenAfterMapClose = true;
                }

                if (auto* queue = RE::UIMessageQueue::GetSingleton()) {
                    queue->AddMessage(RE::MapMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr);
                } else {
                    SKSE::log::error("Could not get the UI message queue to close MapMenu.");
                    std::scoped_lock lock(flowLock);
                    reopenAfterMapClose = false;
                }
                break;

            case FlowMode::kNone:
                SKSE::log::trace("Map choice completed with no pending menu action.");
                break;
        }
    }

    // Returns true only when this close is the intermediate step of replacing one open world map with another.
    bool OnMapMenuClosed()
    {
        {
            std::scoped_lock lock(flowLock);

            if (!reopenAfterMapClose) return false;

            reopenAfterMapClose = false;
        }

        SKSE::log::debug("MapMenu closed during a map switch; scheduling its replacement.");

        if (QueueSelectedMapOpen()) {
            return true;
        } else {
            SKSE::log::error("Could not reopen MapMenu after switching maps.");
            return false;
        }
    }
}
