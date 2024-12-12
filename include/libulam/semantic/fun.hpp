#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/scope/object.hpp>
#include <libulam/semantic/params.hpp>
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
    FunOverload(Ref<ast::FunDef> node): _node{node} {}

    Ref<Type> ret_type() { return _ret_type; }
    Ref<const Type> ret_type() const { return _ret_type; }

    void set_ret_type(Ref<Type> ret_type) { _ret_type = ret_type; }

    // TMP
    auto& params() { return _params; }
    const auto& param() const { return _params; }

    void add_param(Ref<Type> type, Value&& default_value);

    Ref<ast::FunDef> node() { return _node; }
    Ref<ast::TypeName> ret_type_name();
    Ref<ast::ParamList> params_node();
    Ref<ast::FunDefBody> body_node();

private:
    Ref<ast::FunDef> _node;
    Ref<Type> _ret_type{};
    ParamList _params;
};

class Fun : public ScopeObject {
public:
    Fun() {}

    void merge(Ref<Fun> other);

    // TMP
    auto& overloads() { return _overloads; }
    const auto& overloads() const { return _overloads; }

    Ref<FunOverload> add_overload(Ref<ast::FunDef> node, PersScopeState scope_state);
    Ref<FunOverload> add_overload(Ref<FunOverload> overload);

private:
    std::list<RefPtr<FunOverload>> _overloads;
};

} // namespace ulam
