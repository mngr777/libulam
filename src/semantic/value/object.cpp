#include <libulam/semantic/value.hpp>
#include <libulam/semantic/value/object.hpp>
#include <libulam/semantic/type/class.hpp>

namespace ulam {

Object::Object(Ref<Class> cls): _cls{cls}, _bits{cls->bitsize()} {}

Object::~Object() {}

} // namespace ulam
