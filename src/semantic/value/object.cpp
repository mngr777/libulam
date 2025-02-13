#include <cassert>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/value/object.hpp>

namespace ulam {

Object::Object(Ref<Class> cls): _cls{cls}, _bits{cls->bitsize()} {}

Object::Object(Ref<Class> cls, BitVector&& bits):
    _cls{cls}, _bits{std::move(bits)} {
    assert(_bits.len() == cls->bitsize());
}

Object::~Object() {}

} // namespace ulam
