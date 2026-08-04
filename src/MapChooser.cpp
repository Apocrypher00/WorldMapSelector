#include "ClassicMessageBox.h"
#include "Config.h"
#include "MapChooser.h"
#include "MapSelection.h"
#include "MapSwitchFlow.h"
#include "WorldspaceCatalog.h"
#include "WorldspaceOverride.h"

namespace WMS::MapChooser
{
    namespace
    {
        // constexpr makes this a compile-time constant
        // size_t is the unsigned type used for container sizes and indexes.
        constexpr std::size_t mapsPerPage = 6;

        // enum class creates named values without leaking kSelectMap, etc.
        // into the surrounding namespace or allowing accidental integer conversion.
        enum class ActionType
        {
            kClearSelection,
            kSelectMap,
            kPreviousPage,
            kNextPage,
            kCancel
        };

        // One Action records what a particular message-box button should do.
        struct Action
        {
            ActionType type = ActionType::kCancel;
            RE::TESWorldSpace* worldspace = nullptr;
            std::size_t page = 0;
            std::string displayName;
        };

        // MessageBoxMenu callbacks return only a button index,
        // so preserve the matching actions until the callback arrives.
        std::mutex actionsLock;
        std::vector<Action> pendingActions;

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

        std::string MakeButtonLabel(const WorldspaceCatalog::MapOption& option, RE::TESWorldSpace* currentMap, RE::TESWorldSpace* selectedMap)
        {
            auto label = option.displayName;

            if (option.hasDuplicateDisplayName && !option.editorID.empty()) {
                label += fmt::format(" ({})", option.editorID);
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
        // HandleAction calls ShowPage for Previous / Next
        // ShowPage passes OnMessageBoxResult as the message - box callback
        // OnMessageBoxResult calls HandleAction for the selected button
        void ShowPage(std::size_t page);

		// HandleAction performs the action associated with a message-box button.
        // const Action& borrows the existing action without copying it and prevents this function from modifying it.
        void HandleAction(const Action& action)
        {
            switch (action.type) {
                case ActionType::kClearSelection:
                    MapSelection::SelectDefault();
                    RE::SendHUDMessage::ShowHUDMessage("World map selection cleared.");
                    SKSE::log::info("Map chooser cleared the map selection.");
                    MapSwitchFlow::Cancel();
                    break;

                case ActionType::kSelectMap:
                    MapSelection::Select(action.worldspace);
                    RE::SendHUDMessage::ShowHUDMessage(fmt::format("World map selected: {}", action.displayName).c_str());
                    SKSE::log::info("Map chooser selected {} ({:08X}).",
                        action.displayName, action.worldspace ? action.worldspace->GetFormID() : 0
                    );
                    MapSwitchFlow::ApplyAfterChoice();
                    break;

                case ActionType::kPreviousPage:
                case ActionType::kNextPage:
                    ShowPage(action.page);
                    break;

                case ActionType::kCancel:
                    MapSwitchFlow::Cancel();
                    break;
                }
        }

        void OnMessageBoxResult(std::uint8_t button)
        {
            // Start empty because Skyrim may return an out-of-range index.
            std::optional<Action> action;
            std::size_t actionCount = 0;
            {
                // Limit the mutex lifetime to copying one action and clearing
                // shared state; HandleAction runs after the lock is released.
                std::scoped_lock lock(actionsLock);
                actionCount = pendingActions.size();
                if (button < pendingActions.size()) {
                    action = pendingActions[button];
                }
                pendingActions.clear();
            }

            if (action) {
                HandleAction(*action);
            } else {
                SKSE::log::warn("Map chooser returned invalid button index {} for {} actions.", button, actionCount);
            }
        }

        void ShowPage(std::size_t requestedPage)
        {
            auto* currentMap   = WorldspaceCatalog::GetMapOwner(GetCurrentWorldspace());
            auto* selectedMap  = WorldspaceCatalog::GetMapOwner(MapSelection::GetSelectedWorldspace());
            const auto options = WorldspaceCatalog::GetOrderedOptions(currentMap, selectedMap);
            if (options.empty()) {
                MapSwitchFlow::Cancel();
                RE::SendHUDMessage::ShowHUDMessage("WorldMapSelector found no selectable maps.");
                return;
            }

            // Integer ceiling division calculates enough pages for every map.
            // Parentheses around min/max prevent Windows macros from expanding.
            const auto requiredPageCount = (options.size() + mapsPerPage - 1) / mapsPerPage;
            const auto pageCount         = (std::max)(std::size_t{ 1 }, requiredPageCount);
            const auto page              = (std::min)(requestedPage, pageCount - 1);
            const auto first             = page * mapsPerPage;
            const auto last              = (std::min)(first + mapsPerPage, options.size());

            SKSE::log::debug(
                "Opening map chooser page {}/{} with {} selectable maps.",
                page + 1, pageCount, options.size()
            );

            std::vector<std::string> buttons;
            std::vector<Action> actions;

            if (page == 0) {
                buttons.emplace_back("[Clear Selection]");
                // Designated initializers make clear which Action fields this particular button requires;
                // omitted fields keep defaults.
                actions.push_back({
                    .type = ActionType::kClearSelection
                });
            }

            if (page > 0) {
				// The characters &lt; and &gt; are HTML-escaped to avoid confusing the message-box parser.
                buttons.emplace_back("&lt;&lt; Previous");
                actions.push_back({
                    .type = ActionType::kPreviousPage,
                    .page = page - 1
                });
            }

            for (auto index = first; index < last; ++index) {
                const auto& option = options[index];
                const auto label = MakeButtonLabel(option, currentMap, selectedMap);

                buttons.push_back(label);
                actions.push_back({
                    .type = ActionType::kSelectMap,
                    .worldspace = option.worldspace,
                    .displayName = option.displayName
                });
            }

            if (page + 1 < pageCount) {
                // The characters &lt; and &gt; are HTML-escaped to avoid confusing the message-box parser.
                buttons.emplace_back("Next &gt;&gt;");
                actions.push_back({
                    .type = ActionType::kNextPage,
                    .page = page + 1
                });
            }

            buttons.emplace_back("Cancel");
            actions.push_back({
                .type = ActionType::kCancel
            });

            const auto message = fmt::format("Select World Map ({}/{})", page + 1, pageCount);

            {
                std::scoped_lock lock(actionsLock);
                // Transfer the completed vector into callback-visible storage.
                pendingActions = std::move(actions);
            }

            if (!ClassicMessageBox::Open(message.c_str(), buttons, OnMessageBoxResult)) {
                {
                    std::scoped_lock lock(actionsLock);
                    pendingActions.clear();
                }
                SKSE::log::error("Could not open the world-map chooser.");
                MapSwitchFlow::Cancel();
            }
        }

        void OpenChooser()
        {
            auto* player = RE::PlayerCharacter::GetSingleton();
            auto* ui = RE::UI::GetSingleton();
            if (!player || !ui) {
                SKSE::log::debug("Ignored map chooser request because player or UI state is unavailable.");
                return;
            }

            // Never replace another message box's buttons or callback state.
            // This includes fast-travel confirmations and custom-marker menus displayed over MapMenu.
            if (ui->IsMenuOpen(RE::MessageBoxMenu::MENU_NAME)) {
                SKSE::log::trace("Ignored map chooser request because another message box is open.");
                return;
            }

            if (ui->IsMenuOpen(RE::MapMenu::MENU_NAME)) {
                if (!Config::GetAllowChooserWhileMapOpen()) {
                    SKSE::log::debug("Ignored map chooser request while MapMenu is open; AllowChooserWhileMapOpen is false.");
                } else {
                    MapSwitchFlow::ConfigureInsideMap();
                    ShowPage(0);
                }
                return;
            }

            if (ui->GameIsPaused() || ui->IsModalMenuOpen()) {
                SKSE::log::trace("Ignored map chooser request while another paused or modal menu is active.");
                return;
            }

            // Choose whether a selection made outside MapMenu should open it.
            MapSwitchFlow::ConfigureOutsideMap(Config::GetOpenMapAfterSelection());
            ShowPage(0);
        }

    }

    void Open()
    {
        OpenChooser();
    }

    // Classic MessageBoxMenu only focuses its final button on Escape.
    // Select our known Cancel button explicitly so one press dismisses the chooser without changing selection.
    bool SelectCancelButton()
    {
        std::optional<std::int32_t> cancelIndex;
        {
            std::scoped_lock lock(actionsLock);
            if (!pendingActions.empty() && pendingActions.back().type == ActionType::kCancel) {
                // Convert the unsigned vector index to the signed integer expected by SelectOption.
                cancelIndex = static_cast<std::int32_t>(pendingActions.size() - 1);
            }
        }

        if (!cancelIndex) return false;

        SKSE::log::trace("Translated Escape into map chooser Cancel button {}.", *cancelIndex);
        RE::MessageBoxMenu::SelectOption(*cancelIndex);
        return true;
    }
}
