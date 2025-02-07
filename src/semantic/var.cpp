#include <libulam/semantic/var.hpp>

namespace ulam {

void Var::set_value(Value&& value) { std::swap(_value, value); }

} // namespace ulam
