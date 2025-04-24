#pragma once
#include "libulam/semantic/type.hpp"
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/type/class/ancestry.hpp>
#include <libulam/semantic/type/class/base.hpp>
#include <libulam/semantic/type/conv.hpp>
#include <libulam/semantic/value/bits.hpp>
#include <list>
#include <map>
#include <string_view>

namespace ulam::ast {
class ClassDef;
class FunDef;
class TypeName;
class VarDecl;
class VarDefList;
} // namespace ulam::ast

namespace ulam::sema {
class Resolver;
}

namespace ulam {

class ConvList;
class Diag;
class ClassTpl;
class Type;
class Value;

class Class : public UserType, public ClassBase {
    friend ClassTpl;
    friend cls::Ancestry;

public:
    Class(const std::string_view name, Ref<ClassTpl> tpl);
    Class(const std::string_view name, Ref<ast::ClassDef> node, Ref<Module> module);
    ~Class();

    std::string name() const override { return std::string{_name}; }
    str_id_t name_id() const override;

    Ref<Var>
    add_param(Ref<ast::TypeName> type_node, Ref<ast::VarDecl> node) override;
    Ref<Var> add_param(
        Ref<ast::TypeName> type_node, Ref<ast::VarDecl> node, Value&& val);

    Ref<AliasType> add_type_def(Ref<ast::TypeDef> node) override;
    Ref<Fun> add_fun(Ref<ast::FunDef> node) override;
    Ref<Var>
    add_const(Ref<ast::TypeName> type_node, Ref<ast::VarDecl> node) override;
    Ref<Prop>
    add_prop(Ref<ast::TypeName> type_node, Ref<ast::VarDecl> node) override;

    bool init(sema::Resolver& resolver);
    Ref<AliasType> init_type_def(sema::Resolver& resolver, str_id_t name_id);
    bool resolve(sema::Resolver& resolver);

    // NOTE: check existence with `has()/has_op()` first
    Symbol* get_resolved(str_id_t name_id, sema::Resolver& resolver);
    Ref<FunSet> resolved_op(Op op, sema::Resolver& resolver);

    bool is_base_of(Ref<const Class> other) const;
    bool is_same_or_base_of(Ref<const Class> other) const;

    bool has_super() const;
    Ref<Class> super();

    Ref<Class> base_by_name_id(str_id_t name_id);

    bitsize_t base_off(Ref<const Class> base) const;

    const auto& parents() const { return _ancestry.parents(); }
    const auto& ancestors() const { return _ancestry.ancestors(); }

    bitsize_t bitsize() const override;
    bitsize_t required_bitsize() const;
    bitsize_t direct_bitsize() const;

    Ref<cls::Ancestor> first_parent_over_max_bitsize();
    Ref<Prop> first_prop_over_max_bitsize();

    bool is_constructible() const override { return true; }
    RValue construct() override;

    RValue construct(Bits&& bits);

    RValue load(const BitsView data, bitsize_t off) override;
    void store(BitsView data, bitsize_t off, const RValue& rval) override;

    bool is_castable_to(Ref<const Type> type, bool expl = true) const override;
    bool is_castable_to(
        BuiltinTypeId builtin_type_id, bool expl = true) const override;

    Value cast_to(Ref<Type> type, Value&& val) override;

    conv_cost_t
    conv_cost(Ref<const Type> type, bool allow_cast = false) const override;

    ConvList convs(Ref<const Type> type, bool allow_cast = false) const;
    ConvList convs(BuiltinTypeId bi_type_id, bool allow_cast = false) const;

    // TODO: make protected
    void add_conv(Ref<Fun> fun);

    const auto& type_defs() const { return _type_defs; }

    const auto& consts() const { return _consts; }

    const auto& props() const { return _props; }
    const auto& all_props() const { return _all_props; } // + inherited

protected:
    Ref<Class> _as_class() override { return this; }
    Ref<const Class> _as_class() const override { return this; }

    Ref<FunSet> add_fset(str_id_t name_id) override;
    Ref<FunSet> add_op_fset(Op op) override;

private:
    bool resolve_params(sema::Resolver& resolver);
    bool init_ancestors(sema::Resolver& resolver, bool resolve);
    bool resolve_props(sema::Resolver& resolver);
    bool resolve_funs(sema::Resolver& resolver);
    void init_default_data(sema::Resolver& resolver);

    void add_ancestor(Ref<Class> cls, Ref<ast::TypeName> node);

    void merge_fsets();
    void init_layout();

    auto& convs() { return _convs; }
    auto& fsets() { return _fsets; }

    const std::string_view _name;
    Ref<ClassTpl> _tpl;
    cls::Ancestry _ancestry;
    std::list<Ref<AliasType>> _type_defs;
    std::list<Ref<Var>> _consts;
    std::list<Ref<Prop>> _props;
    std::list<Ref<Prop>> _all_props;
    std::map<type_id_t, Ref<Fun>> _convs;
    std::map<str_id_t, Ref<FunSet>> _fsets;
    Bits _init_bits;
};

} // namespace ulam
