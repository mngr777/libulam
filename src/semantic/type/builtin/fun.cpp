#include <libulam/assert.hpp>
#include <libulam/semantic/type/builtin/fun.hpp>

namespace ulam {

bitsize_t FunType::bitsize() const { unreachable(); }

RValue FunType::load(const BitsView data, bitsize_t off) {
    unreachable();
}

void FunType::store(BitsView data, bitsize_t off, const RValue& rval) {
    unreachable();
}

Value FunType::cast_to(Ref<Type> type, Value&& val) { unreachable(); }

Ref<ArrayType> FunType::array_type(array_size_t size) { unreachable(); }

Ref<RefType> FunType::ref_type() { unreachable(); }

} // namespace ulam
