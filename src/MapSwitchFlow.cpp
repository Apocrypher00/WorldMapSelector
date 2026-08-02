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

        void OpenSelectedMap()
        {
            // UI messages are queued onto Skyrim's task thread instead of
            // changing menus directly from an input or message-box callback.
            auto* tasks = SKSE::GetTaskInterface();
            if (!tasks) {
                SKSE::log::error(
                    "Could not queue the selected world map to open.");
                return;
            }

            tasks->AddTask([] {
                if (auto* queue =
                        RE::UIMessageQueue::GetSingleton()) {
                    queue->AddMessage(
                        RE::MapMenu::MENU_NAME,
                        RE::UI_MESSAGE_TYPE::kShow,
                        nullptr);
                }
            });
        }
    }

    void ConfigureOutsideMap(bool openAfterChoice)
    {
        Configure(
            openAfterChoice
                ? FlowMode::kOpenAfterChoice
                : FlowMode::kNone
        );
    }

    void ConfigureInsideMap()
    {
        Configure(FlowMode::kSwitchOpenMap);
    }

    void Cancel()
    {
        Consume();
    }

    void ApplyAfterChoice()
    {
        switch (Consume()) {
        case FlowMode::kOpenAfterChoice:
            OpenSelectedMap();
            break;

        case FlowMode::kSwitchOpenMap:
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

        if (auto* tasks = SKSE::GetTaskInterface()) {
            tasks->AddTask([] {
                OpenSelectedMap();
            });
            return true;
        }

        SKSE::log::error("Could not reopen MapMenu after switching maps.");
        return false;
    }
}
