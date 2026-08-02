#include "ClassicMessageBox.h"

namespace WMS::ClassicMessageBox
{
    bool Open(
        const char* message,
        const std::vector<std::string>& buttons,
        ResultCallback callback)
    {
        // This lambda converts an owned std::string to the const char* expected
        // by Skyrim. The vector keeps those strings alive during Create.
        const auto button = [&](std::size_t index) {
            return buttons[index].c_str();
        };

        // Skyrim's Create function is variadic, so C++ must make a separate
        // call for every supported button count.
        switch (buttons.size()) {
            case 1:
                return RE::MessageBoxMenu::Create(
                    message, callback, 0, 0, 10,
                    button(0)
                );
            case 2:
                return RE::MessageBoxMenu::Create(
                    message, callback, 0, 0, 10,
                    button(0), button(1)
                );
            case 3:
                return RE::MessageBoxMenu::Create(
                    message, callback, 0, 0, 10,
                    button(0), button(1), button(2)
                );
            case 4:
                return RE::MessageBoxMenu::Create(
                    message, callback, 0, 0, 10,
                    button(0), button(1), button(2), button(3)
                );
            case 5:
                return RE::MessageBoxMenu::Create(
                    message, callback, 0, 0, 10,
                    button(0), button(1), button(2), button(3), button(4)
                );
            case 6:
                return RE::MessageBoxMenu::Create(
                    message, callback, 0, 0, 10,
                    button(0), button(1), button(2), button(3), button(4), button(5)
                );
            case 7:
                return RE::MessageBoxMenu::Create(
                    message, callback, 0, 0, 10,
                    button(0), button(1), button(2), button(3), button(4), button(5), button(6)
                );
            case 8:
                return RE::MessageBoxMenu::Create(
                    message, callback, 0, 0, 10,
                    button(0), button(1), button(2), button(3), button(4), button(5), button(6), button(7)
                );
            case 9:
                return RE::MessageBoxMenu::Create(
                    message, callback, 0, 0, 10,
                    button(0), button(1), button(2), button(3), button(4), button(5), button(6), button(7), button(8)
                );
            default:
                return false;
        }
    }
}
