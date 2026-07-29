#include "Config.h"
#include "MapChooser.h"
#include "MapSelection.h"
#include "WorldspaceCatalog.h"
#include "WorldspaceOverride.h"

namespace WMS::MapChooser
{
    namespace
    {
        constexpr std::size_t mapsPerPage = 6;

        enum class ActionType
        {
            kDefault,
            kSelectMap,
            kPreviousPage,
            kNextPage,
            kCancel
        };

        enum class FlowMode
        {
            kNone,
            kOpenAfterChoice,
            kSwitchOpenMap
        };

        struct Action
        {
            ActionType type = ActionType::kCancel;
            RE::TESWorldSpace* worldspace = nullptr;
            std::size_t page = 0;
            std::string displayName;
        };

        std::mutex actionsLock;
        std::vector<Action> pendingActions;

        std::mutex flowLock;
        FlowMode flowMode = FlowMode::kNone;
        bool reopenAfterMapClose = false;

        RE::TESWorldSpace* GetCurrentWorldspace()
        {
            if (auto* worldspace =
                    WorldspaceOverride::GetActualMapWorldspace()) {
                return worldspace;
            }

            if (auto* tes = RE::TES::GetSingleton()) {
                if (auto* worldspace =
                        tes->GetRuntimeData2().worldSpace) {
                    return worldspace;
                }
            }

            if (auto* player =
                    RE::PlayerCharacter::GetSingleton()) {
                return player->GetWorldspace();
            }

            return nullptr;
        }

        bool EqualsIgnoreCase(
            std::string_view left,
            std::string_view right)
        {
            return left.size() == right.size() &&
                   _strnicmp(
                       left.data(),
                       right.data(),
                       left.size()) == 0;
        }

        std::string MakeButtonLabel(
            const WorldspaceCatalog::MapOption& option,
            const std::vector<WorldspaceCatalog::MapOption>& options,
            RE::TESWorldSpace* currentMap)
        {
            auto label = option.displayName;

            const auto duplicateCount =
                std::ranges::count_if(
                    options,
                    [&](const auto& candidate) {
                        return EqualsIgnoreCase(
                            candidate.displayName,
                            option.displayName);
                    });
            if (duplicateCount > 1 &&
                !option.editorID.empty()) {
                label += fmt::format(
                    " ({})",
                    option.editorID);
            }

            if (option.worldspace == currentMap) {
                label += " [Current]";
            }

            return label;
        }

        void ShowPage(std::size_t page);

        void ConfigureFlow(FlowMode mode)
        {
            std::scoped_lock lock(flowLock);
            flowMode = mode;
        }

        FlowMode ConsumeFlow()
        {
            std::scoped_lock lock(flowLock);
            const auto result = flowMode;
            flowMode = FlowMode::kNone;
            return result;
        }

        void OpenSelectedMap()
        {
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

        void ApplyFlowAfterChoice()
        {
            switch (ConsumeFlow()) {
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

        void HandleAction(const Action& action)
        {
            switch (action.type) {
            case ActionType::kDefault:
                MapSelection::SelectDefault();
                RE::SendHUDMessage::ShowHUDMessage(
                    "World map selected: Default");
                SKSE::log::info(
                    "Map chooser selected Default.");
                ApplyFlowAfterChoice();
                break;

            case ActionType::kSelectMap:
                MapSelection::Select(action.worldspace);
                RE::SendHUDMessage::ShowHUDMessage(
                    fmt::format(
                        "World map selected: {}",
                        action.displayName)
                        .c_str());
                SKSE::log::info(
                    "Map chooser selected {} ({:08X}).",
                    action.displayName,
                    action.worldspace
                        ? action.worldspace->GetFormID()
                        : 0);
                ApplyFlowAfterChoice();
                break;

            case ActionType::kPreviousPage:
            case ActionType::kNextPage:
                ShowPage(action.page);
                break;

            case ActionType::kCancel:
                ConsumeFlow();
                break;
            }
        }

        void OnMessageBoxResult(std::uint8_t button)
        {
            std::optional<Action> action;
            {
                std::scoped_lock lock(actionsLock);
                if (button < pendingActions.size()) {
                    action = pendingActions[button];
                }
                pendingActions.clear();
            }

            if (action) {
                HandleAction(*action);
            }
        }

        bool OpenClassicMessageBox(
            const char* message,
            const std::vector<std::string>& buttons)
        {
            const auto button = [&](std::size_t index) {
                return buttons[index].c_str();
            };

            switch (buttons.size()) {
            case 1:
                return RE::MessageBoxMenu::Create(
                    message, OnMessageBoxResult, 0, 0, 10,
                    button(0));
            case 2:
                return RE::MessageBoxMenu::Create(
                    message, OnMessageBoxResult, 0, 0, 10,
                    button(0), button(1));
            case 3:
                return RE::MessageBoxMenu::Create(
                    message, OnMessageBoxResult, 0, 0, 10,
                    button(0), button(1), button(2));
            case 4:
                return RE::MessageBoxMenu::Create(
                    message, OnMessageBoxResult, 0, 0, 10,
                    button(0), button(1), button(2), button(3));
            case 5:
                return RE::MessageBoxMenu::Create(
                    message, OnMessageBoxResult, 0, 0, 10,
                    button(0), button(1), button(2), button(3),
                    button(4));
            case 6:
                return RE::MessageBoxMenu::Create(
                    message, OnMessageBoxResult, 0, 0, 10,
                    button(0), button(1), button(2), button(3),
                    button(4), button(5));
            case 7:
                return RE::MessageBoxMenu::Create(
                    message, OnMessageBoxResult, 0, 0, 10,
                    button(0), button(1), button(2), button(3),
                    button(4), button(5), button(6));
            case 8:
                return RE::MessageBoxMenu::Create(
                    message, OnMessageBoxResult, 0, 0, 10,
                    button(0), button(1), button(2), button(3),
                    button(4), button(5), button(6), button(7));
            case 9:
                return RE::MessageBoxMenu::Create(
                    message, OnMessageBoxResult, 0, 0, 10,
                    button(0), button(1), button(2), button(3),
                    button(4), button(5), button(6), button(7),
                    button(8));
            default:
                return false;
            }
        }
        void ShowPage(std::size_t requestedPage)
        {
            auto* currentMap =
                WorldspaceCatalog::GetMapOwner(
                    GetCurrentWorldspace());
            const auto options =
                WorldspaceCatalog::GetOrderedOptions(
                    currentMap);
            if (options.empty()) {
                RE::SendHUDMessage::ShowHUDMessage(
                    "WorldMapSelector found no selectable maps.");
                return;
            }

            const auto pageCount =
                (std::max)(
                    std::size_t{ 1 },
                    (options.size() + mapsPerPage - 1) /
                        mapsPerPage);
            const auto page =
                (std::min)(requestedPage, pageCount - 1);
            const auto first = page * mapsPerPage;
            const auto last =
                (std::min)(
                    first + mapsPerPage,
                    options.size());

            std::vector<std::string> buttons;
            std::vector<Action> actions;

            if (page == 0) {
                buttons.emplace_back("Default");
                actions.push_back({
                    .type = ActionType::kDefault
                });
            }

            if (page > 0) {
                buttons.emplace_back("&lt;&lt; Previous");
                actions.push_back({
                    .type = ActionType::kPreviousPage,
                    .page = page - 1
                });
            }

            for (auto index = first; index < last; ++index) {
                const auto& option = options[index];
                const auto label =
                    MakeButtonLabel(
                        option,
                        options,
                        currentMap);

                buttons.push_back(label);
                actions.push_back({
                    .type =
                        option.worldspace == currentMap
                            ? ActionType::kDefault
                            : ActionType::kSelectMap,
                    .worldspace = option.worldspace,
                    .displayName = option.displayName
                });
            }

            if (page + 1 < pageCount) {
                buttons.emplace_back("Next >>");
                actions.push_back({
                    .type = ActionType::kNextPage,
                    .page = page + 1
                });
            }

            buttons.emplace_back("Cancel");
            actions.push_back({
                .type = ActionType::kCancel
            });

            const auto message = fmt::format(
                "Select World Map ({}/{})",
                page + 1,
                pageCount);

            {
                std::scoped_lock lock(actionsLock);
                pendingActions = std::move(actions);
            }

            if (!OpenClassicMessageBox(
                    message.c_str(),
                    buttons)) {
                {
                    std::scoped_lock lock(actionsLock);
                    pendingActions.clear();
                }
                SKSE::log::error(
                    "Could not open the world-map chooser.");
                ConsumeFlow();
            }
        }

        void Open()
        {
            Config::Load();

            auto* player =
                RE::PlayerCharacter::GetSingleton();
            auto* ui = RE::UI::GetSingleton();
            if (!player || !ui) {
                return;
            }

            if (ui->IsMenuOpen(RE::MapMenu::MENU_NAME)) {
                if (!Config::GetAllowChooserWhileMapOpen()) {
                    return;
                }

                ConfigureFlow(FlowMode::kSwitchOpenMap);
                ShowPage(0);
                return;
            }

            if (ui->GameIsPaused() ||
                ui->IsModalMenuOpen() ||
                ui->IsMenuOpen(RE::MessageBoxMenu::MENU_NAME)) {
                return;
            }

            ConfigureFlow(
                Config::GetOpenMapAfterSelection()
                    ? FlowMode::kOpenAfterChoice
                    : FlowMode::kNone);
            ShowPage(0);
        }

        class InputEventSink final :
            public RE::BSTEventSink<RE::InputEvent*>
        {
        public:
            RE::BSEventNotifyControl ProcessEvent(
                RE::InputEvent* const* events,
                RE::BSTEventSource<RE::InputEvent*>*) override
            {
                if (!events) {
                    return RE::BSEventNotifyControl::kContinue;
                }

                const auto configuredKey =
                    Config::GetOpenSelectorKey();
                if (configuredKey == 0) {
                    return RE::BSEventNotifyControl::kContinue;
                }

                for (auto* event = *events;
                     event;
                     event = event->next) {
                    const auto* button =
                        event->AsButtonEvent();
                    if (button &&
                        button->device ==
                            RE::INPUT_DEVICE::kKeyboard &&
                        button->GetIDCode() == configuredKey &&
                        button->IsDown()) {
                        Open();
                        break;
                    }
                }

                return RE::BSEventNotifyControl::kContinue;
            }
        };
    }

    bool RegisterInputSink()
    {
        auto* input =
            RE::BSInputDeviceManager::GetSingleton();
        if (!input) {
            SKSE::log::error(
                "Could not get the input-device manager.");
            return false;
        }

        static InputEventSink inputEventSink;
        input->AddEventSink(&inputEventSink);

        SKSE::log::info(
            "Registered map chooser hotkey 0x{:02X}.",
            Config::GetOpenSelectorKey());
        return true;
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

        SKSE::log::error(
            "Could not reopen MapMenu after switching maps.");
        return false;
    }
}
