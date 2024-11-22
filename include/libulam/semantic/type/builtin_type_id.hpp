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
    StringId
};

inline bool is_void_type_id(BuiltinTypeId id) { return id == VoidId; }

inline bool is_prim(BuiltinTypeId id) {
    assert(id != NoBuiltinTypeId);
    return id != AtomId;
}

inline bool has_bitsize(BuiltinTypeId id) {
    assert(id != NoBuiltinTypeId);
    return id != AtomId && id != VoidId && id != StringId;
}

const std::string_view builtin_type_str(BuiltinTypeId id);

} // namespace ulam
