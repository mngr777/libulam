#pragma once
#include <libulam/lang/class.hpp>
#include <libulam/lang/ops.hpp>
#include <libulam/lang/type_ops.hpp>
#include <libulam/src_loc.hpp>
#include <libulam/str_pool.hpp>
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

tok::Type type_by_keyword(const std::string_view str);

Class::Kind class_kind(Type type);

TypeOp type_op(Type type);

Op bin_op(Type type);
Op unary_pre_op(Type type);
Op unary_post_op(Type type);

} // namespace tok

struct Token {
    using size_t = std::uint16_t;

    tok::Type type{tok::Eof};
    size_t size{0};
    loc_id_t loc_id{NoLocId};
    str_id_t str_id{NoStrId};

    const char* type_str() const { return tok::type_str(type); }
    const char* type_name() const { return tok::type_name(type); }

    bool is(tok::Type type_) const { return type_ == type; }

    template <typename T, typename... Ts> bool in(T first, Ts... rest) {
        return is(first) || in(rest...);
    }
    bool in() { return false; }

    Class::Kind class_kind() const { return tok::class_kind(type); }

    TypeOp type_op() const { return tok::type_op(type); }

    Op bin_op() const { return tok::bin_op(type); }
    Op unary_pre_op() const { return tok::unary_pre_op(type); }
    Op unary_post_op() const { return tok::unary_post_op(type); }
};

} // namespace ulam
