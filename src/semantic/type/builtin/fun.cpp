#include "src/utils/unreachable.hpp"
#include <libulam/semantic/type/builtin/fun.hpp>

namespace ulam {

bitsize_t FunType::bitsize() const { utils::unreachable(); }

RValue FunType::load(const BitsView data, bitsize_t off) {
    utils::unreachable();
}

void FunType::store(BitsView data, bitsize_t off, const RValue& rval) {
    utils::unreachable();
}

Value FunType::cast_to(Ref<Type> type, Value&& val) { utils::unreachable(); }

Ref<ArrayType> FunType::array_type(array_size_t size) { utils::unreachable(); }

Ref<RefType> FunType::ref_type() { utils::unreachable(); }

} // namespace ulam
