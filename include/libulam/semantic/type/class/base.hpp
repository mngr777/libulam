#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/fun.hpp>
#include <libulam/semantic/ops.hpp>
#include <libulam/semantic/scope/flags.hpp>
#include <libulam/semantic/symbol.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/class/prop.hpp>
#include <libulam/semantic/type/class_kind.hpp>
#include <libulam/semantic/var.hpp>
#include <list>
#include <map>
#include <string_view>

namespace ulam::ast {
class ClassDef;
class FunDef;
class Param;
class TypeDef;
class TypeName;
class VarDef;
class VarDefList;
} // namespace ulam::ast

namespace ulam::sema {
class Resolver;
}

namespace ulam {

class Module;
class PersScope;
class Value;

class ClassBase {
public:
    using SymbolTable = _SymbolTable<UserType, FunSet, Var, Prop>;
    using Symbol = SymbolTable::Symbol;

    ClassBase(
        Ref<ast::ClassDef> node, Ref<Module> module, scope_flags_t scope_flags);

    ClassBase(ClassBase&&) = default;
    ClassBase& operator=(ClassBase&&) = default;

public:
    ClassKind kind() const;

    bool is_element() const { return kind() == ClassKind::Element; }
    bool is_quark() const { return kind() == ClassKind::Quark; }
    bool is_transient() const { return kind() == ClassKind::Transient; }
    bool is_union() const { return kind() == ClassKind::Union; }

    bool has(str_id_t name_id) const;

    bool has_fun(str_id_t name_id) const;
    bool has_fun(const std::string_view name) const;

    Ref<FunSet> fun(str_id_t name_id);
    Ref<FunSet> fun(const std::string_view name);

    Symbol* get(const std::string_view name);
    const Symbol* get(const std::string_view name) const;

    Symbol* get(str_id_t name_id);
    const Symbol* get(str_id_t name_id) const;

    bool has_op(Op op) const;
    Ref<FunSet> op(Op op);

    virtual Ref<AliasType> add_type_def(Ref<ast::TypeDef> node);

    virtual Ref<Fun> add_fun(Ref<ast::FunDef> node);

    virtual Ref<Var>
    add_const(Ref<ast::TypeName> type_node, Ref<ast::VarDecl> node);

    virtual Ref<Prop>
    add_prop(Ref<ast::TypeName> type_node, Ref<ast::VarDecl> node);

    const auto& params() const { return _params; }

    bool has_constructors() const;
    Ref<FunSet> constructors();

    Ref<ast::ClassDef> node() { return _node; }
    Ref<const ast::ClassDef> node() const { return _node; }

    Ref<PersScope> param_scope() { return ref(_param_scope); }
    Ref<PersScope> scope() { return ref(_scope); }

protected:
    template <typename T> Symbol* set(str_id_t name_id, Ptr<T>&& value) {
        return _members.set(name_id, std::move(value));
    }

    template <typename T> Symbol* set(str_id_t name_id, Ref<T> value) {
        return _members.set(name_id, value);
    }

    Ref<PersScope> inh_scope() { return ref(_inh_scope); }

    auto& members() { return _members; }

    Ref<FunSet> find_fset(str_id_t name_id);
    Ref<FunSet> find_op_fset(Op op);

    virtual Ref<FunSet> add_fset(str_id_t name_id);
    virtual Ref<FunSet> add_op_fset(Op op);

    virtual Ref<Var> add_param(Ptr<Var>&& var);

    auto& ops() { return _ops; }

private:
    Ref<ast::ClassDef> _node;
    Ref<Module> _module;
    // TODO: store in class
    Ptr<PersScope> _inh_scope;
    Ptr<PersScope> _param_scope;
    Ptr<PersScope> _scope;
    SymbolTable _members;
    std::list<Ref<Var>> _params;
    Ptr<FunSet> _constructors;
    std::map<Op, Ptr<FunSet>> _ops;
};

} // namespace ulam
