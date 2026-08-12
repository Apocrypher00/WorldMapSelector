#include "Config.h"
#include "MapMenuKeyHint.h"

namespace WMS::MapMenuKeyHint
{
    namespace
    {
        bool GetSkyUIButtonPanel(RE::GFxMovieView* movie, RE::GFxValue& buttonPanel, RE::GFxValue& buttons)
        {
            // Detect the interface actually exposed by the loaded MapMenu movie.
            // This is safer than checking for SkyUI.esp because another UI mod may replace MapMenu.swf.
            if (!movie->GetVariable(&buttonPanel, "_root.bottomBar.buttonPanel") || !buttonPanel.IsDisplayObject()) {
                return false;
            }

            return buttonPanel.GetMember("buttons", &buttons) && buttons.IsArray();
        }

        bool IsButtonConfigured(const RE::GFxValue& button)
        {
            // SkyUI's clearButtons method empties each label during a platform rebuild.
            // A retained label therefore means our entry is already present and needs no work.
            RE::GFxValue label;
            return button.GetMember("label", &label) && label.IsString() &&
                   std::string_view(label.GetString()) == "Select Map";
        }

        bool CreateSkyUIButton(RE::GFxValue& buttonPanel, RE::GFxValue& buttons, RE::GFxValue& button)
        {
            // Reuse the clip created during an earlier call. SkyUI keeps attached clips and
            // its public array when it clears and repopulates buttons for a platform change.
            if (buttonPanel.GetMember("worldMapSelectorButton", &button) && button.IsDisplayObject()) {
                return true;
            }

            // Reuse SkyUI's renderer and initializer so the new button inherits the active UI skin.
            RE::GFxValue renderer;
            RE::GFxValue initializer;
            RE::GFxValue depth;
            if (!buttonPanel.GetMember("buttonRenderer", &renderer) || !renderer.IsString() ||
                !buttonPanel.GetMember("buttonInitializer", &initializer) ||
                !buttonPanel.Invoke("getNextHighestDepth", &depth) || !depth.IsNumber()) {
                return false;
            }

            // Attach one additional button clip to SkyUI's panel and register it in the public array.
            // SkyUI's addButton method can then treat it as the eighth normal panel entry.
            const RE::GFxValue* initializerArgument = initializer.IsObject() ? &initializer : nullptr;
            if (!buttonPanel.AttachMovie(
                    &button,
                    renderer.GetString(),
                    "worldMapSelectorButton",
                    static_cast<std::int32_t>(depth.GetSInt()),
                    initializerArgument) ||
                !button.IsDisplayObject() || !buttons.PushBack(button)) {
                return false;
            }

            return buttonPanel.SetMember("maxButtons", buttons.GetArraySize());
        }

        void CreateButtonData(RE::GFxMovieView* movie, RE::GFxValue& buttonData)
        {
            // Describe the configured key using the same object format as SkyUI's built-in hints.
            RE::GFxValue controls;
            movie->CreateObject(&controls);
            controls.SetMember("keyCode", Config::GetOpenSelectorKey());

            movie->CreateObject(&buttonData);
            buttonData.SetMember("text", "Select Map");
            buttonData.SetMember("controls", controls);
        }
    }

    void Show()
    {
        if (!Config::GetAllowChooserWhileMapOpen() || !Config::GetShowMapMenuKeyHint()) {
            return;
        }

        auto* ui = RE::UI::GetSingleton();
        if (!ui) {
            SKSE::log::warn("Could not get the UI singleton while adding the MapMenu key hint.");
            return;
        }

        const auto mapMenu = ui->GetMenu<RE::MapMenu>();
        if (!mapMenu || !mapMenu->uiMovie) {
            SKSE::log::warn("MapMenu movie was unavailable while adding the key hint.");
            return;
        }

        auto* movie = mapMenu->uiMovie.get();
        RE::GFxValue buttonPanel;
        RE::GFxValue buttons;
        if (!GetSkyUIButtonPanel(movie, buttonPanel, buttons)) {
            SKSE::log::debug("MapMenu does not expose SkyUI's expected button panel; key hint was skipped.");
            return;
        }

        RE::GFxValue button;
        if (!CreateSkyUIButton(buttonPanel, buttons, button)) {
            SKSE::log::warn("Could not extend SkyUI's MapMenu button panel.");
            return;
        }

        if (IsButtonConfigured(button)) {
            return;
        }

        RE::GFxValue buttonData;
        CreateButtonData(movie, buttonData);

        // Add the new entry through SkyUI's normal population and layout methods.
        RE::GFxValue configuredButton;
        if (!buttonPanel.Invoke("addButton", &configuredButton, std::array { buttonData }) ||
            !configuredButton.IsDisplayObject() ||
            !buttonPanel.Invoke("updateButtons", nullptr, std::array { RE::GFxValue(true) })) {
            SKSE::log::warn("SkyUI MapMenu key hint could not be configured.");
            return;
        }

        SKSE::log::debug("Added a separate SkyUI MapMenu key hint for scan code 0x{:02X}.", Config::GetOpenSelectorKey());
    }

    void RefreshAfterInputModeChange()
    {
        // Input events also arrive during gameplay. Only queue UI work when MapMenu is active.
        auto* ui = RE::UI::GetSingleton();
        if (!ui || !ui->IsMenuOpen(RE::MapMenu::MENU_NAME)) {
            return;
        }

        // Run after the current input dispatch finishes so SkyUI can rebuild its platform-specific
        // button list first. Show is idempotent and exits when the hint is already configured.
        const auto* tasks = SKSE::GetTaskInterface();
        if (!tasks) {
            SKSE::log::warn("Could not schedule the MapMenu key hint after an input-mode change.");
            return;
        }

        tasks->AddUITask(Show);
    }
}
