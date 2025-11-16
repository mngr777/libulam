#pragma once
#include <libulam/semantic/value/types.hpp>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>

namespace ulam::detail {

void write_leximited(std::ostream& os, Integer value);
void write_leximited(std::ostream& os, Unsigned value);
void write_leximited(std::ostream& os, std::string_view value);
void write_leximited(std::ostream& os, char value);
void write_leximited(std::ostream& os, bool value);

template <typename T> std::string leximited(T value) {
    std::ostringstream ss;
    write_leximited(ss, value);
    return ss.str();
}

} // namespace ulam::detail
