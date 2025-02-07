#pragma once
#include <libulam/semantic/decl.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/class/layout.hpp>
#include <libulam/semantic/type_tpl.hpp>
#include <libulam/semantic/typed_value.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/str_pool.hpp>

namespace ulam::ast {
class TypeName;
class VarDecl;
} // namespace ulam::ast

namespace ulam {

// TODO: data member/parameter/... subclasses?

class Var : public Decl {
public:
    using Flag = std::uint8_t;
    static constexpr Flag NoFlags = 0;
    static constexpr Flag Const = 1;
    static constexpr Flag ClassParam = 1 << 1;
    static constexpr Flag FunParam = 1 << 2;
    static constexpr Flag Tpl = 1 << 3;

    Var(Ref<ast::TypeName> type_node,
        Ref<ast::VarDecl> node,
        Ref<Type> type,
        Flag flags = NoFlags):
        _type_node{type_node}, _node{node}, _type{type}, _flags{flags} {}

    Var(Ref<ast::TypeName> type_node,
        Ref<ast::VarDecl> node,
        TypedValue&& tv,
        Flag flags = NoFlags):
        _type_node{type_node},
        _node{node},
        _type{tv.type()},
        _value{tv.move_value()},
        _flags{flags} {}

    str_id_t name_id() const;

    bitsize_t bitsize() const;

    bool is(Flag flags) const { return (_flags & flags) == flags; }
    bool is_const() const { return is(Const); }

    bool requires_value() const {
        return is_const() && !(is(Tpl) && is(ClassParam)) && !is(FunParam);
    }

    Ref<ast::TypeName> type_node() { return _type_node; }
    Ref<ast::VarDecl> node() { return _node; }

    bool has_type() const { return _type; }
    Ref<Type> type();
    Ref<const Type> type() const;
    void set_type(Ref<Type> type);

    Value& value() { return _value; } // TMP
    const Value& value() const { return _value; }
    void set_value(Value&& value);

    // TODO: if reference, get rvalue from refd var
    RValue* rvalue() { return _value.rvalue(); };

    Flag flags() { return _flags; }

    bool has_data_off() const { return _data_off == cls::NoDataOff; }
    cls::data_off_t data_off() const { return _data_off; }
    void set_data_off(cls::data_off_t off) { _data_off = off; }

private:
    Ref<ast::TypeName> _type_node;
    Ref<ast::VarDecl> _node;
    Ref<Type> _type{};
    Value _value;
    Flag _flags;
    cls::data_off_t _data_off{cls::NoDataOff};
};

} // namespace ulam
