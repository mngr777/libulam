#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/type/class/ancestry.hpp>
#include <libulam/semantic/type/class/base.hpp>
#include <libulam/semantic/type/conv.hpp>
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

    void add_ancestor(Ref<Class> cls, Ref<ast::TypeName> node);

    bool is_base_of(Ref<const Class> other) const;

    auto& parents() { return _ancestry.parents(); }
    const auto& parents() const { return _ancestry.parents(); }

    bitsize_t bitsize() const override;
    bitsize_t direct_bitsize() const;

    Ref<Class> as_class() override { return this; }
    Ref<const Class> as_class() const override { return this; }

    RValue construct() const override;

    RValue load(const BitVectorView data, BitVector::size_t off) const override;
    void store(BitVectorView data, BitVector::size_t off, const RValue& rval)
        const override;

    bool is_castable_to(Ref<const Type> type, bool expl = true) const override;
    bool is_castable_to(
        BuiltinTypeId builtin_type_id, bool expl = true) const override;

    conv_cost_t
    conv_cost(Ref<const Type> type, bool allow_cast = false) const override;

    ConvList convs(Ref<const Type> type, bool allow_cast = false) const;
    ConvList convs(BuiltinTypeId bi_type_id, bool allow_cast = false) const;

    void add_conv(Ref<Fun> fun);

private:
    std::string_view _name;
    Ref<ClassTpl> _tpl;
    cls::Ancestry _ancestry;
    std::map<type_id_t, Ref<Fun>> _convs;
};

} // namespace ulam
