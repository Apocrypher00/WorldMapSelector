#pragma once

namespace WMS::Config
{
    enum class MapSelection
    {
        kActual,
        kOpposite
    };

    MapSelection ReadMapSelection();
    std::string_view ToString(MapSelection selection);
}
