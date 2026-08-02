#pragma once

// CommonLib's umbrella headers expose Skyrim's reverse-engineered types and
// SKSE's plugin interfaces to every source file that uses this precompiled header.
#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

// Makes standard literal suffixes such as "text"sv available without writing
// std::literals:: each time. CommonLib's generated source expects this suffix.
using namespace std::literals;
