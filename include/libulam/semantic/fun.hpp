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
    FunOverload(ast::Ref<ast::FunDef> node): _node{node} {}

    Ref<Type> ret_type() { return _ret_type; }
    Ref<const Type> ret_type() const { return _ret_type; }

    void set_ret_type(Ref<Type> ret_type) { _ret_type = ret_type; }

    // TMP
    auto& param_types() { return _param_types; }
    const auto& param_types() const { return _param_types; }

    void add_param_type(Ref<Type> type);

    ast::Ref<ast::FunDef> node() { return _node; }
    ast::Ref<ast::TypeName> ret_type_name();
    ast::Ref<ast::ParamList> params_node();
    ast::Ref<ast::FunDefBody> body_node();

private:
    ast::Ref<ast::FunDef> _node;
    Ref<Type> _ret_type{};
    std::list<Ref<Type>> _param_types;
};

class Fun : public ScopeObject {
public:
    Fun() {}

    void merge(Ref<Fun> other);

    // TMP
    auto& overloads() { return _overloads; }
    const auto& overloads() const { return _overloads; }

    Ref<FunOverload> add_overload(ast::Ref<ast::FunDef> node, PersScopeState scope_state);
    Ref<FunOverload> add_overload(Ref<FunOverload> overload);

private:
    std::list<RefPtr<FunOverload>> _overloads;
};

} // namespace ulam
