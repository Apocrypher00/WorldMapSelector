#pragma once

namespace WMS::Utilities
{
	bool EqualsIgnoreCase(std::string_view left, std::string_view right);
	bool HasHexPrefix(std::string_view text);
}
