#include "Utilities.h"

namespace WMS::Utilities
{
    // Compare two strings for equality, ignoring case.
    bool EqualsIgnoreCase(std::string_view left, std::string_view right)
    {
        if (left.size() != right.size()) return false;

		// It's unsafe to call _strnicmp with a null pointer.
		if (left.empty()) return true;

        return _strnicmp(left.data(), right.data(), left.size()) == 0;
    }

	// Check if a string starts with the hexadecimal prefix, ignoring case.
	bool HasHexPrefix(std::string_view text)
	{
        return text.starts_with("0x") || text.starts_with("0X");
	}
}
