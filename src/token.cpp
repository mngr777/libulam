#include "libulam/token.hpp"
#include "src/detail/str.hpp"
#include <cassert>
#include <map>

namespace ulam::tok {
namespace {

auto make_word_type_table() {
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

Type type_by_word(std::string_view str) {
    static std::map<std::string_view, Type> types{make_word_type_table()};
    assert(str[0] == '@' || detail::is_word(str[0]));
    auto it = types.find(str);
    if (it != types.end())
        return it->second;
    if (str[0] == '@')
        return InvalidAtKeyword;
    if (detail::is_upper(str[0]))
        return TypeName;
    return Name;
}

Class::Kind class_kind(Type type) {
    switch (type) {
    case Element:
        return Class::Element;
    case Quark:
        return Class::Quark;
    case Transient:
        return Class::Transient;
    default:
        assert(false);
    }
}

Op bin_op(Type type) {
    switch (type) {
    case Dot:
        return Op::MemberAccess;
    case Plus:
        return Op::Sum;
    case Minus:
        return Op::Diff;
    case Ast:
        return Op::Prod;
    case Slash:
        return Op::Quot;
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
