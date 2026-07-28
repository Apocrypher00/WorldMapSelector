#include "Config.h"

namespace
{
    const auto configPath = std::filesystem::absolute("Data\\SKSE\\Plugins\\WorldMapSelector.ini").string();
}

namespace WMS::Config
{
    MapSelection ReadMapSelection()
    {
        std::array<char, 32> value{};
        REX::W32::GetPrivateProfileStringA(
            "WorldMapSelector",
            "MapSelection",
            "Actual",
            value.data(),
            static_cast<DWORD>(value.size()),
            configPath.c_str()
        );

        if (_stricmp(value.data(), "Opposite") == 0) {
            return MapSelection::kOpposite;
        }

        if (_stricmp(value.data(), "Actual") != 0) {
            SKSE::log::warn(
                "Unknown MapSelection value \"{}\"; using Actual.",
                value.data()
            );
        }

        return MapSelection::kActual;
    }

    std::string_view ToString(MapSelection selection)
    {
        switch (selection) {
            case MapSelection::kOpposite:
                return "Opposite";
            default:
                return "Actual";
        }
    }
}
