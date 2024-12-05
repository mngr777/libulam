#pragma once
#include <libulam/ast/ptr.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/scope/object.hpp>
#include <list>

namespace ulam::ast {
class FunDef;
class FunDefBody;
class ParamList;
class TypeName;
} // namespace ulam::ast

namespace ulam {

// TODO: how should functions be stored in scope/class/class tpl?

class Type;

class FunOverload : public ScopeObject {
public:
    FunOverload(ast::Ref<ast::FunDef> node) {}

    ast::Ref<ast::FunDef> node() { return _node; }
    ast::Ref<ast::TypeName> ret_type_node();
    ast::Ref<ast::ParamList> params_node();
    ast::Ref<ast::FunDefBody> body_node();

private:
    ast::Ref<ast::FunDef> _node;
    Ref<Type> _ret_type;
};

class Fun : public ScopeObject {
public:
    Fun() {}

    Ref<FunOverload> add_overload(ast::Ref<FunOverload> overload);
    Ref<FunOverload> add_overload(ast::Ref<ast::FunDef> node);

private:
    std::list<RefPtr<FunOverload>> _overloads;
};

} // namespace ulam
