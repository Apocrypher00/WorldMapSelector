#include "Config.h"

namespace
{
    const auto configPath = std::filesystem::absolute("Data\\SKSE\\Plugins\\WorldMapSelector.ini").string();
}

namespace WMS::Config
{
    std::string ReadMapSelection()
    {
        std::array<char, 256> value{};
        REX::W32::GetPrivateProfileStringA(
            "WorldMapSelector",
            "MapSelection",
            "Default",
            value.data(),
            static_cast<DWORD>(value.size()),
            configPath.c_str()
        );

        return value.data();
    }
}
