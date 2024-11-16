#include <cassert>
#include <libulam/semantic/type/builtin_type_id.hpp>

namespace ulam {

const std::string_view builtin_type_str(BuiltinTypeId id) {
    switch (id) {
    case IntId:
        return "Int";
    case UnsignedId:
        return "Unisgned";
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

} // namespace ulam
