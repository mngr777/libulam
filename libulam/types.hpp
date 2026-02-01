#pragma once
#include <cstdint>
#include <filesystem>
#include <list>

namespace ulam {

using version_t = std::uint32_t;
constexpr version_t DefaultVersion = 5;

using Path = std::filesystem::path;
using PathList = std::list<Path>;

} // namespace ulam
