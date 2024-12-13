#pragma once
#include <libulam/semantic/scope/object.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type_tpl.hpp>
#include <libulam/semantic/typed_value.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/str_pool.hpp>

namespace ulam::ast {
class TypeName;
class VarDecl;
} // namespace ulam::ast

namespace ulam {

class Var : public ScopeObject {
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

    bool requires_value() const {
        return is_const() && !(is(Tpl) && is(ClassParam)) && !is(FunParam);
    }

    Ref<ast::TypeName> type_node() { return _type_node; }
    Ref<ast::VarDecl> node() { return _node; }

    Ref<Type> type() { return _type; }
    Ref<const Type> type() const { return _type; }
    void set_type(Ref<Type> type);

    Value& value() { return _value; }

    // TODO: if reference, get rvalue from references var
    RValue* rvalue() { return _value.rvalue(); };

    bool is_const() const { return _flags & Const; }

    Flag flags() { return _flags; }

private:
    Ref<ast::TypeName> _type_node;
    Ref<ast::VarDecl> _node;
    Ref<Type> _type{};
    Value _value;
    Flag _flags;
};

} // namespace ulam
