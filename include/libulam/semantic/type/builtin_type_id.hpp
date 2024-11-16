#pragma once
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

const std::string_view builtin_type_str(BuiltinTypeId id);

} // namespace ulam
