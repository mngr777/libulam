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

class Type;

class FunOverload : public ScopeObject {
public:
    FunOverload(ast::Ref<ast::FunDef> node) {}

    void add_param_type(Ref<Type> type);

    ast::Ref<ast::FunDef> node() { return _node; }
    ast::Ref<ast::TypeName> ret_type_node();
    ast::Ref<ast::ParamList> params_node();
    ast::Ref<ast::FunDefBody> body_node();

private:
    ast::Ref<ast::FunDef> _node;
    Ref<Type> _ret_type;
    std::list<Ref<Type>> _param_types;
};

class Fun : public ScopeObject {
public:
    Fun() {}

    // TMP
    auto& overloads() { return _overloads; }
    const auto& overloads() const { return _overloads; }

    Ref<FunOverload> add_overload(ast::Ref<ast::FunDef> node);

private:
    std::list<Ptr<FunOverload>> _overloads;
};

} // namespace ulam
