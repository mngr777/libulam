#pragma once
#include <libulam/lang/string.hpp>
#include <string_view>

namespace ulam::detail {

String parse_str(const std::string_view str);

}
