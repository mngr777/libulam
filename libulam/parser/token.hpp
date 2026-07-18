#pragma once
#include <libulam/semantic/ops.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/type/class_kind.hpp>
#include <libulam/semantic/type/class_name_kind.hpp>
#include <libulam/semantic/type_ops.hpp>
#include <libulam/src_loc.hpp>
#include <libulam/src_man.hpp>
#include <string_view>

namespace ulam {
namespace tok {

enum Type : std::uint8_t {
#define TOK(str, type) type,
#include "token.inc.hpp"
};

using flags_t = std::uint8_t;
namespace flags {
constexpr flags_t NoFlags = 0;
constexpr flags_t HasMeta = 1;
} // namespace flags

// NOTE: `type_str' may return nullptr
const char* type_str(Type type);
const char* type_name(Type type);

tok::Type type_by_keyword(const std::string_view str);

ClassKind class_kind(Type type);

bool is_class_name(Type type);
ClassNameKind class_name_kind(Type type);

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

class Token {
public:
    using size_t = std::uint16_t;

    Token() {}
    Token(
        SrcMan& src_man,
        SrcLoc loc,
        tok::Type type,
        const char* ptr,
        size_t size):
        _src_man(&src_man),
        _loc{loc},
        _type{type},
        _orig_type{type},
        _ptr{ptr},
        _size{size} {}

    const char* type_str() const { return tok::type_str(_type); }
    const char* type_name() const { return tok::type_name(_type); }

    bool is(tok::Type type) const { return type == _type; }

    template <typename T, typename... Ts> bool in(T first, Ts... rest) {
        return is(first) || in(rest...);
    }
    bool in() { return false; }

    ClassKind class_kind() const { return tok::class_kind(_type); }

    bool is_class_name() const { return tok::is_class_name(_orig_type); }
    ClassNameKind class_name_kind() const {
        return tok::class_name_kind(_orig_type);
    }

    BuiltinTypeId builtin_type_id() const {
        return tok::builtin_type_id(_orig_type);
    }

    bool is_type_op() const { return tok::is_type_op(_type); }
    TypeOp type_op() const { return tok::type_op(_type); }

    bool is_op() const { return tok::is_op(_type); }
    bool is_overloadable_op() const { return tok::is_overloadable_op(_type); }

    Op bin_op() const { return tok::bin_op(_type); }
    Op unary_pre_op() const { return tok::unary_pre_op(_type); }
    Op unary_post_op() const { return tok::unary_post_op(_type); }

    bool is_self() const { return _orig_type == tok::Self; }
    bool is_self_class() const { return _orig_type == tok::SelfClass; }

    bool is_super() const { return _orig_type == tok::Super; }
    bool is_super_class() const { return _orig_type == tok::SuperClass; }

    tok::Type type() const { return _type; }
    void change_type(tok::Type type);

    const SrcLoc& loc() const { return _loc; }
    tok::Type orig_type() const { return _orig_type; }
    const char* ptr() const { return _ptr; }
    size_t size() const { return _size; }

    loc_id_t loc_id() const;

    bool has_flag(tok::flags_t flag) const { return _flags & flag; }
    void set_flag(tok::flags_t flag) { _flags |= flag; }

private:
    SrcMan* _src_man;
    SrcLoc _loc;
    tok::Type _type{tok::Eof};
    tok::Type _orig_type{tok::Eof};
    const char* _ptr{};
    size_t _size{0};
    tok::flags_t _flags{tok::flags::NoFlags};

    mutable loc_id_t _loc_id{NoLocId};
};

} // namespace ulam
