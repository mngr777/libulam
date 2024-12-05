#pragma once
#include <cassert>
#include <libulam/ast/ptr.hpp>
#include <libulam/semantic/scope/object.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type_tpl.hpp>
#include <libulam/semantic/typed_value.hpp>
#include <libulam/semantic/value.hpp>

namespace ulam::ast {
class TypeName;
class VarDecl;
} // namespace ulam::ast

namespace ulam {

class Var : public ScopeObject {
public:
    using Flag = std::uint8_t;
    static constexpr Flag NoFlags = 0;
    static constexpr Flag IsConst = 1;
    static constexpr Flag ClassParam = 1 << 1 | IsConst;

    Var(ast::Ref<ast::TypeName> type_node,
        ast::Ref<ast::VarDecl> node,
        Ref<Type> type,
        Flag flags = NoFlags):
        _type_node{type_node}, _node{node}, _type{type}, _flags{flags} {}

    Var(ast::Ref<ast::TypeName> type_node,
        ast::Ref<ast::VarDecl> node,
        TypedValue&& tv,
        Flag flags = NoFlags):
        _type_node{type_node},
        _node{node},
        _type{tv.type()},
        _value{tv.move_value()} {}

    bool is(Flag flag) const { return _flags & flag; }

    ast::Ref<ast::TypeName> type_node() { return _type_node; }
    ast::Ref<ast::VarDecl> node() { return _node; }

    Ref<Type> type() { return _type; }

    void set_type(Ref<Type> type) {
        assert(!_type);
        _type = type;
    }

    Value& value() { return _value; }

    // TODO: if reference, get rvalue from references var
    RValue* rvalue() { return _value.rvalue(); };

    bool is_const() const { return _flags & IsConst; }

    Flag flags() { return _flags; }

private:
    ast::Ref<ast::TypeName> _type_node;
    ast::Ref<ast::VarDecl> _node;
    Ref<Type> _type;
    Value _value;
    Flag _flags;
};

} // namespace ulam
