#pragma once

namespace WMS::ClassicMessageBox
{
    using ResultCallback = void(*)(std::uint8_t);
    bool Open(const char* message, const std::vector<std::string>& buttons, ResultCallback callback);
    bool Dismiss();
}
