#include "Config.h"
#include "Utilities.h"

#include <charconv>
#include <SimpleIni.h>

namespace WMS::Config
{
    namespace
    {
        // Turn the game-relative INI path into the path SimpleIni reads.
        const auto configPath = std::filesystem::absolute("Data\\SKSE\\Plugins\\WorldMapSelector.ini").string();

        // Initialize the plugin configuration settings with their default values.
        // Missing or invalid INI entries leave these values unchanged.
        auto logLevel                 = spdlog::level::info; // This default actually needs to be applied
        std::uint32_t openSelectorKey = 0x44;
        bool openMapAfterSelection    = true;
        bool persistSelection         = true;
        bool allowChooserWhileMapOpen = true;

        std::optional<spdlog::level::level_enum> ParseLogLevel(std::string_view text)
        {
            if (Utilities::EqualsIgnoreCase(text, "trace"))    return spdlog::level::trace;
            if (Utilities::EqualsIgnoreCase(text, "debug"))    return spdlog::level::debug;
            if (Utilities::EqualsIgnoreCase(text, "info"))     return spdlog::level::info;
            if (Utilities::EqualsIgnoreCase(text, "warn"))     return spdlog::level::warn;
            if (Utilities::EqualsIgnoreCase(text, "err"))      return spdlog::level::err;
            if (Utilities::EqualsIgnoreCase(text, "critical")) return spdlog::level::critical;
            if (Utilities::EqualsIgnoreCase(text, "off"))      return spdlog::level::off;
            return std::nullopt;
        }

        std::optional<std::uint32_t> ParseKeyCode(std::string_view text)
        {
            // Require the hexadecimal prefix documented in the INI,
            // then remove it from this non-owning view before parsing the remaining digits.
            if (!Utilities::HasHexPrefix(text)) return std::nullopt;
            text.remove_prefix(2);
            if (text.empty()) return std::nullopt;

            std::uint32_t parsed = 0;
            const char* start    = text.data();
            const char* end      = start + text.size();
            const auto result    = std::from_chars(start, end, parsed, 16);

            // from_chars reports both a parsing error and where parsing stopped.
            // Require every character to be valid hex and the result to fit in one byte.
            if (result.ec != std::errc{} || result.ptr != end || parsed > 0xFF) {
                return std::nullopt;
            }

            return parsed;
        }

        std::optional<bool> ParseBool(std::string_view text)
        {
            if (Utilities::EqualsIgnoreCase(text, "true"))  return true;
            if (Utilities::EqualsIgnoreCase(text, "false")) return false;
            return std::nullopt;
        }

        // Set the global log level and flush policy for spdlog.
        void ApplyLogLevel()
        {
            spdlog::set_level(logLevel);
            spdlog::default_logger()->flush_on(logLevel);
        }

        void ReadLogLevel(const CSimpleIniA& ini)
        {
            if (const char* configured = ini.GetValue("General", "LogLevel")) {
                if (const auto parsed = ParseLogLevel(configured)) {
                    logLevel = *parsed;
                }
                else { SKSE::log::warn("Invalid LogLevel \"{}\"; keeping default Info.", configured); }
            }
            else { SKSE::log::debug("LogLevel not specified; keeping default Info."); }

            ApplyLogLevel();
        }

        void ReadSelectorKey(const CSimpleIniA& ini)
        {
            if (const char* configured = ini.GetValue("Controls", "OpenSelectorKey")) {
                if (const auto parsed = ParseKeyCode(configured)) {
                    openSelectorKey = *parsed;
                }
                else { SKSE::log::warn("Invalid OpenSelectorKey \"{}\"; keeping default 0x44 (F10).", configured); }
            }
            else { SKSE::log::debug("OpenSelectorKey not specified; keeping default 0x44 (F10)."); }
        }

        void ReadBool(const CSimpleIniA& ini, const char* key, bool& setting)
        {
            if (const char* configured = ini.GetValue("Behavior", key)) {
                if (const auto parsed = ParseBool(configured)) {
                    setting = *parsed;
                }
                else { SKSE::log::warn("Invalid {} \"{}\"; expected true or false. Keeping default.", key, configured); }
            }
            else { SKSE::log::debug("{} not specified. Keeping default.", key); }
        }
    }

    // Load the INI file and replace defaults only with settings that exist and parse successfully.
    void Load()
    {
		// Read the INI file into memory.
		// SimpleIni's LoadFile returns a status code, which is negative on error and non-negative on success.
        CSimpleIniA ini;
        const auto loadResult = ini.LoadFile(configPath.c_str());
        if (loadResult < SI_OK) {
            SKSE::log::warn("Could not load {}; keeping all default settings (error {}).", configPath, loadResult);
			ApplyLogLevel();
            return;
        }

		// Read each setting from the INI file, leaving the default value unchanged if the key is missing or invalid.
        ReadLogLevel(ini);
		ReadSelectorKey(ini);
        ReadBool(ini, "OpenMapAfterSelection", openMapAfterSelection);
        ReadBool(ini, "AllowChooserWhileMapOpen", allowChooserWhileMapOpen);
        ReadBool(ini, "PersistSelection", persistSelection);

        SKSE::log::info(
            "Loaded configuration: LogLevel={}, OpenSelectorKey=0x{:02X}, OpenMapAfterSelection={}, AllowChooserWhileMapOpen={}, PersistSelection={}.",
            spdlog::level::to_string_view(logLevel), openSelectorKey, openMapAfterSelection, allowChooserWhileMapOpen, persistSelection
        );
    }

    // Public getters return copies so callers cannot modify the stored settings.
    std::uint32_t GetOpenSelectorKey() { return openSelectorKey; }
    bool GetOpenMapAfterSelection()    { return openMapAfterSelection; }
    bool GetPersistSelection()         { return persistSelection; }
    bool GetAllowChooserWhileMapOpen() { return allowChooserWhileMapOpen; }
}
