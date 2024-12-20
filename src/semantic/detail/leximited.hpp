#pragma once
#include <libulam/semantic/value.hpp>
#include <ostream>
#include <string_view>

namespace ulam::detail {

void write_leximited(std::ostream& os, Integer value);
void write_leximited(std::ostream& os, Unsigned value);
void write_leximited(std::ostream& os, String& value);
void write_leximited(std::ostream& os, std::string_view value);
void write_leximited(std::ostream& os, char value);
void write_leximited(std::ostream& os, bool value);

} // namespace ulam::detail
