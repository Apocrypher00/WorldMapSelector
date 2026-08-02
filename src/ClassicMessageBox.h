#pragma once

namespace WMS::ClassicMessageBox
{
    using ResultCallback = void(*)(std::uint8_t);

    // Open displays one classic Skyrim message box and reports the selected
    // button index through callback.
    bool Open(
        const char* message,
        const std::vector<std::string>& buttons,
        ResultCallback callback);
}
