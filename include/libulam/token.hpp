#pragma once
#include <libulam/semantic/ops.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/type/class_kind.hpp>
#include <libulam/semantic/type_ops.hpp>
#include <libulam/src_loc.hpp>
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

ClassKind class_kind(Type type);

// TODO: shorten
bool is_builtin_type_id(Type type);
BuiltinTypeId builtin_type_id(Type type);

bool is_type_op(Type type);
TypeOp type_op(Type type);

bool is_op(Type type);
bool is_overloadable_op(Type type);
Op bin_op(Type type);
Op unary_pre_op(Type type);
Op unary_post_op(Type type);

} // namespace tok

struct Token {
    using size_t = std::uint16_t;

    tok::Type type{tok::Eof};
    tok::Type orig_type{tok::Eof};
    size_t size{0};
    loc_id_t loc_id{NoLocId};

    const char* type_str() const { return tok::type_str(type); }
    const char* type_name() const { return tok::type_name(type); }

    bool is(tok::Type type_) const { return type_ == type; }

    template <typename T, typename... Ts> bool in(T first, Ts... rest) {
        return is(first) || in(rest...);
    }
    bool in() { return false; }

    ClassKind class_kind() const { return tok::class_kind(type); }

    BuiltinTypeId builtin_type_id() const {
        return tok::builtin_type_id(orig_type);
    }

    bool is_type_op() const { return tok::is_type_op(type); }
    TypeOp type_op() const { return tok::type_op(type); }

    bool is_op() const { return tok::is_op(type); }
    bool is_overloadable_op() const { return tok::is_overloadable_op(type); }

    Op bin_op() const { return tok::bin_op(type); }
    Op unary_pre_op() const { return tok::unary_pre_op(type); }
    Op unary_post_op() const { return tok::unary_post_op(type); }

    bool is_self() const { return orig_type == tok::Self; }
    bool is_self_class() const { return orig_type == tok::SelfClass; }

    bool is_super() const { return orig_type == tok::Super; }
    bool is_super_class() const { return orig_type == tok::SuperClass; }
};

} // namespace ulam
