#include <cassert>
#include <libulam/semantic/type/builtin_type_id.hpp>

namespace ulam {

const std::string_view builtin_type_str(BuiltinTypeId id) {
    switch (id) {
    case IntId:
        return "Int";
    case UnsignedId:
        return "Unsigned";
    case BoolId:
        return "Bool";
    case UnaryId:
        return "Unary";
    case BitsId:
        return "Bits";
    case AtomId:
        return "Atom";
    case VoidId:
        return "Void";
    case StringId:
        return "String";
    default:
        assert(false);
    }
}

const char builtin_type_code(BuiltinTypeId id) {
    switch (id) {
    case IntId:
        return 'i';
    case UnsignedId:
        return 'u';
    case BoolId:
        return 'b';
    case UnaryId:
        return 'y';
    case BitsId:
        return 't';
    case AtomId:
        return 'a';
    case VoidId:
        return 'v';
    case StringId:
        return 's';
    default:
        assert(false);
    }
}

} // namespace ulam
