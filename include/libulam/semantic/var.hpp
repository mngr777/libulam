#pragma once
#include <libulam/ast/ptr.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type_tpl.hpp>
#include <libulam/semantic/value.hpp>

namespace ulam::ast {
class TypeName;
class VarDecl;
} // namespace ulam::ast

namespace ulam {

class Var {
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

    ast::Ref<ast::TypeName> type_node() { return _type_node; }
    ast::Ref<ast::VarDecl> node() { return _node; }

    Ref<Type> type() { return _type; }

    Value& value() { return _value; }

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
