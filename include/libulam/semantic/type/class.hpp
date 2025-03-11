#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/type/class/ancestry.hpp>
#include <libulam/semantic/type/class/base.hpp>
#include <libulam/semantic/type/conv.hpp>
#include <list>
#include <map>
#include <string_view>

namespace ulam::ast {
class ArgList;
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
    Class(std::string_view name, Ref<ClassTpl> tpl);
    Class(std::string_view name, Ref<ast::ClassDef> node, Ref<Module> module);
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

    bool is_base_of(Ref<const Class> other) const;
    bool is_same_or_base_of(Ref<const Class> other) const;

    Ref<Class> base(str_id_t name_id);

    bool has_super() const;
    Ref<Class> super();

    bitsize_t base_off(Ref<const Class> base) const;

    auto& parents() { return _ancestry.parents(); }
    const auto& parents() const { return _ancestry.parents(); }

    bitsize_t bitsize() const override;
    bitsize_t required_bitsize() const;
    bitsize_t direct_bitsize() const;

    Ref<cls::Ancestor> first_parent_over_max_bitsize();
    Ref<Prop> first_prop_over_max_bitsize();

    bool is_constructible() const override { return true; }
    RValue construct() const override;

    RValue load(const BitsView data, bitsize_t off) const override;
    void store(BitsView data, bitsize_t off, const RValue& rval)
        const override;

    bool is_castable_to(Ref<const Type> type, bool expl = true) const override;
    bool is_castable_to(
        BuiltinTypeId builtin_type_id, bool expl = true) const override;

    bool is_castable_to_object_type(Ref<const Type> type, bool expl = false) const;

    Value cast_to_object_type(Ref<const Type> type, Value&& val) const;

    // TODO void write_base_data();

    conv_cost_t
    conv_cost(Ref<const Type> type, bool allow_cast = false) const override;

    ConvList convs(Ref<const Type> type, bool allow_cast = false) const;
    ConvList convs(BuiltinTypeId bi_type_id, bool allow_cast = false) const;

    // TODO: make protected
    void add_conv(Ref<Fun> fun);

protected:
    Ref<Class> _as_class() override { return this; }
    Ref<const Class> _as_class() const override { return this; }

    Ref<FunSet> add_fset(str_id_t name_id) override;
    Ref<FunSet> add_op_fset(Op op) override;

private:
    bool resolve_params(sema::Resolver& resolver);
    bool init_ancestors(sema::Resolver& resolver, bool resolve);
    bool resolve_members(sema::Resolver& resolver);

    void add_ancestor(Ref<Class> cls, Ref<ast::TypeName> node);

    void merge_fsets();
    void init_layout();

    const auto& props() const { return _props; }
    const auto& all_props() const { return _props; } // + inherited
    auto& fsets() { return _fsets; }

    std::string_view _name;
    Ref<ClassTpl> _tpl;
    cls::Ancestry _ancestry;
    std::list<Ref<Prop>> _props;
    std::list<Ref<Prop>> _all_props;
    std::map<type_id_t, Ref<Fun>> _convs;
    std::map<str_id_t, Ref<FunSet>> _fsets;
};

} // namespace ulam
