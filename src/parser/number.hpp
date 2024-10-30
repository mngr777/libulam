#pragma once
#include <libulam/lang/number.hpp>
#include <libulam/parser.hpp>
#include <string>
#include <string_view>
#include <utility>

namespace ulam::detail {

struct ParseNumStatus {
    enum Error { Ok, Incomplete, InvalidDigit, InvalidSuffix };
    Error error{Ok}; // NOTE: client may also check the number for overflow
    std::size_t off{0};
};

std::pair<Number, ParseNumStatus>
parse_num_str(Parser& parser, const std::string_view str);

} // namespace ulam::detail
