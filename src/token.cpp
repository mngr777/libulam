#include "libulam/token.hpp"
#include "src/detail/string.hpp"
#include <cassert>
#include <map>

namespace ulam::tok {
namespace {

auto make_keyword_type_table() {
    // "_dummy_" keys are never inserted, they are only here to keep
    // compiler happy
    std::map<std::string_view, Type> table;
#define TOK(str, type)                                                         \
    if (str && (*((char*)str) == '@' || detail::is_word(*(char*)str)))         \
        table[str ? str : "_dummy_"] = type;
#include "libulam/token.inc.hpp"
#undef TOK
    return table;
}

} // namespace

const char* type_str(tok::Type type) {
    switch (type) {
#define TOK(str, type)                                                         \
    case type:                                                                 \
        return str;
#include "libulam/token.inc.hpp"
#undef TOK
    default:
        assert(false && "Unknown token type");
    }
}

const char* type_name(tok::Type type) {
    switch (type) {
#define TOK(str, type)                                                         \
    case type:                                                                 \
        return #type;
#include "libulam/token.inc.hpp"
#undef TOK
    default:
        assert(false && "Unknown token type");
    }
}

Type type_by_keyword(std::string_view str) {
    static std::map<std::string_view, Type> table{make_keyword_type_table()};
    assert(str[0] == '@' || detail::is_word(str[0]));
    auto it = table.find(str);
    if (it != table.end())
        return it->second;
    if (str[0] == '@')
        return InvalidAtKeyword;
    if (detail::is_upper(str[0]))
        return TypeIdent;
    return Ident;
}

ClassKind class_kind(Type type) {
    switch (type) {
    case Element:
        return ClassKind::Element;
    case Quark:
        return ClassKind::Quark;
    case Transient:
        return ClassKind::Transient;
    default:
        assert(false);
    }
}

BuiltinTypeId builtin_type_id(Type type) {
    switch (type) {
    case IntT:
        return IntId;
    case UnsignedT:
        return UnsignedId;
    case BoolT:
        return BoolId;
    case UnaryT:
        return UnaryId;
    case BitsT:
        return BitsId;
    case AtomT:
        return AtomId;
    case VoidT:
        return VoidId;
    case StringT:
        return StringId;
    default:
        return NoBuiltinTypeId;
    }
}

TypeOp type_op(Type type) {
    switch (type) {
    case MinOf:
        return TypeOp::MinOf;
    case MaxOf:
        return TypeOp::MaxOf;
    case SizeOf:
        return TypeOp::SizeOf;
    case LengthOf:
        return TypeOp::LengthOf;
    case ClassIdOf:
        return TypeOp::ClassIdOf;
    case ConstantOf:
        return TypeOp::ConstantOf;
    case PositionOf:
        return TypeOp::PositionOf;
    case AtomOf:
        return TypeOp::AtomOf;
    case InstanceOf:
        return TypeOp::InstanceOf;
    default:
        return TypeOp::None;
    }
}

Op bin_op(Type type) {
    switch (type) {
    case Period:
        return Op::MemberAccess;
    case Plus:
        return Op::Sum;
    case Minus:
        return Op::Diff;
    case Ast:
        return Op::Prod;
    case Slash:
        return Op::Quot;
    case Less:
        return Op::Less;
    case LessEqual:
        return Op::LessOrEq;
    case Greater:
        return Op::Greater;
    case GreaterEqual:
        return Op::GreaterOrEq;
    default:
        return Op::None;
    }
}

Op unary_pre_op(Type type) {
    switch (type) {
    case Plus:
        return Op::UnaryPlus;
    case Minus:
        return Op::UnaryMinus;
    case PlusPlus:
        return Op::PreInc;
    case MinusMinus:
        return Op::PreDec;
    default:
        return Op::None;
    }
}

Op unary_post_op(Type type) {
    switch (type) {
    case PlusPlus:
        return Op::PostInc;
    case MinusMinus:
        return Op::PostDec;
    default:
        return Op::None;
    }
}

} // namespace ulam::tok
