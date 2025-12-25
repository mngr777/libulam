#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/type/class/ancestry.hpp>
#include <libulam/semantic/type/class/base.hpp>
#include <libulam/semantic/type/conv.hpp>
#include <libulam/semantic/value/bits.hpp>
#include <libulam/semantic/value/types.hpp>
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
class ClassResolver;
} // namespace ulam::sema

namespace ulam {

class ConvList;
class Diag;
class ClassTpl;
class Program;
class Type;
class Value;

class Class : public UserType, public ClassBase {
    friend ClassTpl;
    friend cls::Ancestry;
    friend sema::ClassResolver;

public:
    using UserType::is_castable_to;

    explicit Class(Ref<ClassTpl> tpl);
    Class(Ref<ast::ClassDef> node, Ref<Module> module);
    ~Class();

    const std::string_view name() const override;
    str_id_t name_id() const override;

    const std::string_view full_name() const;
    const std::string_view mangled_name() const;

    cls_id_t class_id() const;
    elt_id_t element_id() const;

    Ref<Var> add_param(Ptr<Var>&& var) override;
    Ref<AliasType> add_type_def(Ref<ast::TypeDef> node) override;
    Ref<Fun> add_fun(Ref<ast::FunDef> node) override;
    Ref<Var>
    add_const(Ref<ast::TypeName> type_node, Ref<ast::VarDecl> node) override;
    Ref<Prop>
    add_prop(Ref<ast::TypeName> type_node, Ref<ast::VarDecl> node) override;

    bool is_base_of(Ref<const Class> other) const;
    bool is_same_or_base_of(Ref<const Class> other) const;

    bool has_super() const;
    Ref<Class> super();

    Ref<Class> base_by_name_id(str_id_t name_id);

    bitsize_t base_off(Ref<const Class> base) const;

    const auto& parents() const { return _ancestry.parents(); }
    const auto& ancestors() const { return _ancestry.ancestors(); }

    bitsize_t bitsize() const override;
    bitsize_t data_bitsize() const;
    bitsize_t required_bitsize() const;
    bitsize_t direct_bitsize() const;
    bitsize_t max_bitsize() const;
    bitsize_t data_off() const;

    Ref<cls::Ancestor> first_parent_over_max_bitsize();
    Ref<Prop> first_prop_over_max_bitsize();

    bool is_constructible() const override { return true; }
    RValue construct() override;
    RValue construct(Bits&& bits);
    RValue construct_ph() override;

    RValue load(const BitsView data, bitsize_t off) override;
    void store(BitsView data, bitsize_t off, const RValue& rval) override;

    TypedValue type_op(TypeOp op) override;

    Ref<Type> common(Ref<Type> type) override;
    Ref<Type>
    common(const Value& val1, Ref<Type> type, const Value& val2) override;

    bool is_castable_to(
        Ref<const Type> type,
        const Value& val,
        bool expl = true) const override;

    bool is_castable_to(
        BuiltinTypeId builtin_type_id,
        const Value& val,
        bool expl = true) const override;

    bool is_refable_as(Ref<const Type> type, const Value& val, bool expl = true)
        const override;

    bool
    is_assignable_to(Ref<const Type> type, const Value& val) const override;

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
    void init(Ref<Module> module);

    void register_class();

    void register_element();
    elt_id_t read_element_id(const BitsView data, bitsize_t off = 0);

    void add_ancestor(Ref<Class> cls, Ref<ast::TypeName> node);

    void merge_fsets();
    void init_layout();
    void set_init_bits(Bits&& bits);

    auto& convs() { return _convs; }
    auto& fsets() { return _fsets; }

    Ref<Program> program() const;

    cls_id_t _cls_id{NoClassId};
    elt_id_t _elt_id{NoEltId};
    Ref<ClassTpl> _tpl;
    cls::Ancestry _ancestry;
    std::list<Ref<AliasType>> _type_defs;
    std::list<Ref<Var>> _consts;
    std::list<Ref<Prop>> _props;
    std::list<Ref<Prop>> _all_props;
    std::map<type_id_t, Ref<Fun>> _convs;
    std::map<str_id_t, Ref<FunSet>> _fsets;
    Bits _init_bits;
    mutable std::string _full_name;
    mutable std::string _mangled_name;
};

} // namespace ulam
