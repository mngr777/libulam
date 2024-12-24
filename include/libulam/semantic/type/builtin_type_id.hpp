#pragma once
#include <cassert>
#include <string_view>

namespace ulam {

enum BuiltinTypeId {
    NoBuiltinTypeId,
    IntId,
    UnsignedId,
    BoolId,
    UnaryId,
    BitsId,
    AtomId,
    VoidId,
    StringId,
    FunId
};

constexpr bool is_prim(BuiltinTypeId id) {
    assert(id != NoBuiltinTypeId);
    return id != AtomId && id != FunId;
}

constexpr bool has_bitsize(BuiltinTypeId id) {
    assert(id != NoBuiltinTypeId);
    return id != AtomId && id != VoidId && id != StringId && id != FunId;
}

const std::string_view builtin_type_str(BuiltinTypeId id);

const char builtin_type_code(BuiltinTypeId id);

} // namespace ulam
