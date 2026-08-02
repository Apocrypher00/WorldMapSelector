#include "Config.h"
#include "MapChooser.h"
#include "MapSelection.h"
#include "Utilities.h"
#include "WorldspaceCatalog.h"
#include "WorldspaceOverride.h"

namespace WMS::MapChooser
{
    namespace
    {
        // constexpr makes this a compile-time constant; size_t is the unsigned
        // type used for container sizes and indexes.
        constexpr std::size_t mapsPerPage = 6;

        // enum class creates named values without leaking kSelectMap, etc. into
        // the surrounding namespace or allowing accidental integer conversion.
        enum class ActionType
        {
            kClearSelection,
            kSelectMap,
            kPreviousPage,
            kNextPage,
            kCancel
        };

        // The same chooser can be opened independently or over MapMenu.
        // Remember what should happen only after a map button is chosen.
        enum class FlowMode
        {
            kNone,
            kOpenAfterChoice,
            kSwitchOpenMap
        };

        // One Action records what a particular message-box button should do.
        struct Action
        {
            ActionType type = ActionType::kCancel;
            RE::TESWorldSpace* worldspace = nullptr;
            std::size_t page = 0;
            std::string displayName;
        };

        // MessageBoxMenu callbacks return only a button index, so preserve the
        // matching actions until the callback arrives.
        std::mutex actionsLock;
        std::vector<Action> pendingActions;

        // Map switching is asynchronous: close the old MapMenu, wait for its
        // close event, and only then open a freshly initialized replacement.
        std::mutex flowLock;
        FlowMode flowMode = FlowMode::kNone;
        bool reopenAfterMapClose = false;

        // Return the most authoritative available source, falling back as needed.
        RE::TESWorldSpace* GetCurrentWorldspace()
        {
            if (auto* worldspace = WorldspaceOverride::GetActualMapWorldspace()) {
                return worldspace;
            }

            if (auto* tes = RE::TES::GetSingleton()) {
                if (auto* worldspace = tes->GetRuntimeData2().worldSpace) {
                    return worldspace;
                }
            }

            if (auto* player = RE::PlayerCharacter::GetSingleton()) {
                return player->GetWorldspace();
            }

            return nullptr;
        }

        std::string MakeButtonLabel(
            const WorldspaceCatalog::MapOption& option,
            const std::vector<WorldspaceCatalog::MapOption>& options,
            RE::TESWorldSpace* currentMap,
            RE::TESWorldSpace* selectedMap)
        {
            auto label = option.displayName;

            // count_if calls this capturing lambda once per option. [&] permits
            // it to read option from the surrounding function by reference.
            const auto duplicateCount =
                std::ranges::count_if(
                    options,
                    [&](const auto& candidate) {
                        return Utilities::EqualsIgnoreCase(
                            candidate.displayName,
                            option.displayName);
                    });
            if (duplicateCount > 1 &&
                !option.editorID.empty()) {
                label += fmt::format(
                    " ({})",
                    option.editorID);
            }

            const bool isHere     = option.worldspace == currentMap;
            const bool isSelected = option.worldspace == selectedMap;

            if (isHere && isSelected) {
                label += " [Here/Selected]";
            } else if (isHere) {
                label += " [Here]";
            } else if (isSelected) {
                label += " [Selected]";
            }

            return label;
        }

        // Forward declaration: HandleAction uses ShowPage before its full definition appears later in this file.
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
            // Return the previous mode while leaving the stored mode reset.
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

            // [] introduces a lambda with no captured variables. SKSE executes
            // this function later on its task thread.
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
            // switch compares one enum value and executes its matching case.
            switch (ConsumeFlow()) {
            case FlowMode::kOpenAfterChoice:
                OpenSelectedMap();
                break;

            case FlowMode::kSwitchOpenMap:
                // Braces give this case its own scope for the lock variable.
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
            // const Action& borrows the existing action without copying it and
            // prevents this function from modifying it.
            switch (action.type) {
            case ActionType::kClearSelection:
                MapSelection::SelectDefault();
                RE::SendHUDMessage::ShowHUDMessage(
                    "World map selection cleared.");
                SKSE::log::info(
                    "Map chooser cleared the map selection.");
                ConsumeFlow();
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
                    // Print the selected FormID, or zero if the pointer is null.
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
            // Start empty because Skyrim may return an out-of-range index.
            std::optional<Action> action;
            {
                // Limit the mutex lifetime to copying one action and clearing
                // shared state; HandleAction runs after the lock is released.
                std::scoped_lock lock(actionsLock);
                if (button < pendingActions.size()) {
                    action = pendingActions[button];
                }
                pendingActions.clear();
            }

            if (action) {
                // Dereference the optional to pass its contained Action.
                HandleAction(*action);
            }
        }

        bool OpenClassicMessageBox(const char* message, const std::vector<std::string>& buttons)
        {
            // This lambda converts an owned std::string to the const char* expected by Skyrim.
            // The vector keeps those strings alive here.
            const auto button = [&](std::size_t index) {
                return buttons[index].c_str();
            };

            // Skyrim's Create function is variadic, so C++ must make a separate call for every supported button count.
            switch (buttons.size()) {
                case 1:
                    return RE::MessageBoxMenu::Create(
                        message, OnMessageBoxResult, 0, 0, 10,
                        button(0)
                    );
                case 2:
                    return RE::MessageBoxMenu::Create(
                        message, OnMessageBoxResult, 0, 0, 10,
                        button(0), button(1)
                    );
                case 3:
                    return RE::MessageBoxMenu::Create(
                        message, OnMessageBoxResult, 0, 0, 10,
                        button(0), button(1), button(2)
                    );
                case 4:
                    return RE::MessageBoxMenu::Create(
                        message, OnMessageBoxResult, 0, 0, 10,
                        button(0), button(1), button(2), button(3)
                    );
                case 5:
                    return RE::MessageBoxMenu::Create(
                        message, OnMessageBoxResult, 0, 0, 10,
                        button(0), button(1), button(2), button(3), button(4)
                    );
                case 6:
                    return RE::MessageBoxMenu::Create(
                        message, OnMessageBoxResult, 0, 0, 10,
                        button(0), button(1), button(2), button(3), button(4), button(5)
                    );
                case 7:
                    return RE::MessageBoxMenu::Create(
                        message, OnMessageBoxResult, 0, 0, 10,
                        button(0), button(1), button(2), button(3), button(4), button(5), button(6)
                    );
                case 8:
                    return RE::MessageBoxMenu::Create(
                        message, OnMessageBoxResult, 0, 0, 10,
                        button(0), button(1), button(2), button(3), button(4), button(5), button(6), button(7)
                    );
                case 9:
                    return RE::MessageBoxMenu::Create(
                        message, OnMessageBoxResult, 0, 0, 10,
                        button(0), button(1), button(2), button(3), button(4), button(5), button(6), button(7), button(8)
                    );
                default:
                    return false;
            }
        }

        void ShowPage(std::size_t requestedPage)
        {
            auto* currentMap   = WorldspaceCatalog::GetMapOwner(GetCurrentWorldspace());
            auto* selectedMap  = WorldspaceCatalog::GetMapOwner(MapSelection::GetSelectedWorldspace());
            const auto options = WorldspaceCatalog::GetOrderedOptions(currentMap, selectedMap);
            if (options.empty()) {
                RE::SendHUDMessage::ShowHUDMessage("WorldMapSelector found no selectable maps.");
                return;
            }

            // Integer ceiling division calculates enough pages for every map.
            // Parentheses around min/max prevent Windows macros from expanding.
            const auto pageCount = (std::max)(
                std::size_t{ 1 },
                (options.size() + mapsPerPage - 1) / mapsPerPage
            );
            const auto page = (std::min)(requestedPage, pageCount - 1);
            const auto first = page * mapsPerPage;
            const auto last = (std::min)(first + mapsPerPage, options.size());

            std::vector<std::string> buttons;
            std::vector<Action> actions;

            if (page == 0) {
                buttons.emplace_back("[Clear Selection]");
                actions.push_back({
                    .type = ActionType::kClearSelection
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
                const auto label = MakeButtonLabel(option, options, currentMap, selectedMap);

                buttons.push_back(label);
                // Designated initializers make clear which Action fields this
                // particular button requires; omitted fields keep defaults.
                actions.push_back({
                    .type = ActionType::kSelectMap,
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
                // Transfer the completed vector into callback-visible storage.
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

            // Choose whether a selection made outside MapMenu should open it.
            ConfigureFlow(
                Config::GetOpenMapAfterSelection()
                    ? FlowMode::kOpenAfterChoice
                    : FlowMode::kNone
            );
            ShowPage(0);
        }

        // The sink receives linked lists of input events from Skyrim.
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

                // events points to the first pointer in Skyrim's linked list.
                // Each event->next advances until a null pointer ends the list.
                for (auto* event = *events;
                     event;
                     event = event->next) {
                    // AsButtonEvent returns null when this input event is not a button.
                    const auto* button =
                        event->AsButtonEvent();
                    if (!button ||
                        button->device !=
                            RE::INPUT_DEVICE::kKeyboard ||
                        !button->IsDown()) {
                        continue;
                    }

                    // Classic MessageBoxMenu only focuses its final button on
                    // Escape. Explicitly invoke our known Cancel action so one
                    // press dismisses the chooser without changing selection.
                    if (button->GetIDCode() == 0x01) {
                        std::optional<std::int32_t> cancelIndex;
                        {
                            std::scoped_lock lock(actionsLock);
                            if (!pendingActions.empty() &&
                                pendingActions.back().type ==
                                    ActionType::kCancel) {
                                // Convert the unsigned vector index to the signed
                                // integer expected by SelectOption.
                                cancelIndex = static_cast<std::int32_t>(pendingActions.size() - 1);
                            }
                        }

                        if (cancelIndex) {
                            // * extracts the integer held by the optional.
                            RE::MessageBoxMenu::SelectOption(*cancelIndex);
                            return RE::BSEventNotifyControl::kStop;
                        }
                    }

                    if (configuredKey != 0 &&
                        button->GetIDCode() == configuredKey) {
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

        // Static lifetime keeps the registered sink alive for the whole process.
        static InputEventSink inputEventSink;
        // & passes the sink's address to Skyrim rather than copying the object.
        input->AddEventSink(&inputEventSink);

        SKSE::log::info(
            "Registered map chooser hotkey 0x{:02X}.",
            Config::GetOpenSelectorKey());
        return true;
    }

    bool OnMapMenuClosed()
    {
        // Returning true tells the menu event sink that this close belongs to
        // a switch, not to the end of a one-shot selection.
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
