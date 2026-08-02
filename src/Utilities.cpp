#include "Utilities.h"

namespace WMS::Utilities
{
    // Compare two strings for equality, ignoring case.
    bool EqualsIgnoreCase(std::string_view left, std::string_view right)
    {
        // _strnicmp performs a case-insensitive comparison and returns zero when both strings match.
        return (
            left.size() == right.size() &&
            _strnicmp(left.data(), right.data(), left.size()) == 0
        );
    }
}