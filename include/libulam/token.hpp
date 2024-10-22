#pragma once
#include "libulam/lang/class.hpp"
#include "libulam/lang/ops.hpp"
#include "libulam/src_loc.hpp"
#include <string_view>

namespace ulam {
namespace tok {

enum Type : std::uint8_t {
#define TOK(str, type) type,
#include "token.inc.hpp"
#undef TOK
};

// NOTE: `type_str' may return nullptr
const char* type_str(Type type);
const char* type_name(Type type);

// NOTE: '@keyword' is allowed
tok::Type type_by_word(const std::string_view str);

Class::Kind class_kind(Type type);

Op bin_op(Type type);
Op unary_pre_op(Type type);
Op unary_post_op(Type type);

} // namespace tok

using str_id_t = std::uint32_t; // TODO: move to str_pool
constexpr str_id_t NoStrId = -1;

struct Token {
    using size_t = std::uint16_t;

    tok::Type type{tok::Eof};
    size_t size{0};
    loc_id_t loc_id{NoLocId};
    str_id_t str_id{NoStrId};

    const char* type_str() const { return tok::type_str(type); }
    const char* type_name() const { return tok::type_name(type); }

    bool is(tok::Type typ) const { return typ == type; }
    bool is(tok::Type typ1, tok::Type typ2) const {
        return is(typ1) || is(typ2);
    }

    template <typename T, typename... Ts> bool in(T first, Ts... rest) {
        return is(first) && in(rest...);
    }

    Class::Kind class_kind() const { return tok::class_kind(type); }

    Op bin_op() const { return tok::bin_op(type); }
    Op unary_pre_op() const { return tok::bin_op(type); }
    Op unary_post_op() const { return tok::bin_op(type); }
};

} // namespace ulam
