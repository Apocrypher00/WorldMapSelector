#include "Utilities.h"

namespace WMS::Utilities
{
    // Compare two strings for equality, ignoring case.
    bool EqualsIgnoreCase(std::string_view left, std::string_view right)
    {
        if (left.size() != right.size()) {
            return false;
        }

        // Avoid passing the potentially null data pointer of an empty
        // string_view to the C runtime comparison function.
        return left.empty() ||
               _strnicmp(left.data(), right.data(), left.size()) == 0;
    }
}
