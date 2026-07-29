#include "Config.h"

namespace
{
    const auto configPath = std::filesystem::absolute("Data\\SKSE\\Plugins\\WorldMapSelector.ini").string();

    std::uint32_t openSelectorKey = 0x44;

    std::string ReadString(
        const char* section,
        const char* key,
        const char* defaultValue)
    {
        std::array<char, 256> value{};
        REX::W32::GetPrivateProfileStringA(
            section,
            key,
            defaultValue,
            value.data(),
            static_cast<DWORD>(value.size()),
            configPath.c_str());

        return value.data();
    }

    std::optional<spdlog::level::level_enum> ParseLogLevel(
        std::string_view value)
    {
        if (_stricmp(value.data(), "trace") == 0) {
            return spdlog::level::trace;
        }
        if (_stricmp(value.data(), "debug") == 0) {
            return spdlog::level::debug;
        }
        if (_stricmp(value.data(), "info") == 0) {
            return spdlog::level::info;
        }
        if (_stricmp(value.data(), "warn") == 0 ||
            _stricmp(value.data(), "warning") == 0) {
            return spdlog::level::warn;
        }
        if (_stricmp(value.data(), "error") == 0) {
            return spdlog::level::err;
        }
        if (_stricmp(value.data(), "critical") == 0) {
            return spdlog::level::critical;
        }
        if (_stricmp(value.data(), "off") == 0) {
            return spdlog::level::off;
        }

        return std::nullopt;
    }

    std::optional<std::uint32_t> ParseKeyCode(std::string_view value)
    {
        char* end = nullptr;
        const auto parsed = std::strtoul(value.data(), &end, 0);
        if (end == value.data() ||
            *end != '\0' ||
            parsed > 0xFF) {
            return std::nullopt;
        }

        return static_cast<std::uint32_t>(parsed);
    }
}

namespace WMS::Config
{
    void Load()
    {
        const auto configuredLogLevel =
            ReadString("General", "LogLevel", "Info");
        const auto logLevel = ParseLogLevel(configuredLogLevel);
        spdlog::set_level(
            logLevel.value_or(spdlog::level::info));
        spdlog::default_logger()->flush_on(
            logLevel.value_or(spdlog::level::info));

        if (!logLevel) {
            SKSE::log::warn(
                "Unknown LogLevel \"{}\"; using Info.",
                configuredLogLevel);
        }

        const auto configuredKey =
            ReadString("Controls", "OpenSelectorKey", "0x44");
        const auto key = ParseKeyCode(configuredKey);
        openSelectorKey = key.value_or(0x44);

        if (!key) {
            SKSE::log::warn(
                "Invalid OpenSelectorKey \"{}\"; using 0x44 (F10).",
                configuredKey);
        }
    }

    std::uint32_t GetOpenSelectorKey()
    {
        return openSelectorKey;
    }
}
