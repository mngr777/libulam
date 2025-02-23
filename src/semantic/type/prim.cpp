#include <libulam/semantic/type/conv.hpp>
#include <cassert>
#include <libulam/semantic/type/prim.hpp>
#include <sstream>

namespace ulam {

// PrimType

PrimType::PrimType(Builtins& builtins, TypeIdGen* id_gen):
    Type{id_gen}, _builtins{builtins} {
    assert(id_gen);
}

std::string PrimType::name() const {
    std::stringstream ss;
    ss << builtin_type_str(builtin_type_id());
    ss << "(" << (Unsigned)bitsize() << ")";
    return ss.str();
}

RValue
PrimType::load(const BitVectorView data, BitVector::size_t off) const {
    return from_datum(data.read(off, bitsize()));
}
void PrimType::store(BitVectorView data, BitVector::size_t off, const RValue& rval)
    const {
    data.write(off, bitsize(), to_datum(rval));
}

bool PrimType::is_castable_to(Ref<const Type> type, bool expl) const {
    auto canon = type->canon();
    return canon->is_prim() ? is_castable_to_prim(canon->as_prim(), expl)
        : false;
}

bool PrimType::is_castable_to(
    BuiltinTypeId builtin_type_id, bool expl) const {
    return false;
}

conv_cost_t PrimType::conv_cost(Ref<const Type> type, bool allow_cast) const {
    if (is_same(type))
        return 0;
    if (!is_castable_to(type, allow_cast))
        return MaxConvCost;
    assert(canon()->is_prim() && type->canon()->is_prim());
    return prim_conv_cost(canon()->as_prim(), type->canon()->as_prim());
}

// PrimTypeTpl

PrimTypeTpl::PrimTypeTpl(Builtins& builtins, TypeIdGen& id_gen):
    TypeTpl{id_gen}, _builtins{builtins} {}

} // namespace ulam
