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
class TypeName;
} // namespace ulam::ast

namespace ulam {

class ConvList;
class Diag;
class ClassTpl;
class Type;

class Class : public UserType, public ClassBase {
    friend ClassTpl;
    friend cls::Ancestry;

public:
    using ParamVarList = std::list<Ref<Var>>;

    Class(TypeIdGen* id_gen, std::string_view name, Ref<ClassTpl> tpl);
    Class(
        TypeIdGen* id_gen,
        std::string_view name,
        Ref<ast::ClassDef> node,
        Ref<Scope> scope);
    ~Class();

    std::string name() const override { return std::string{_name}; }
    str_id_t name_id() const override;

    const ParamVarList& param_vars() const { return _param_vars; }

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

    void add_param_var(Ptr<Var>&& var);
    void add_conv(Ref<Fun> fun);

private:
    std::string_view _name;
    Ref<ClassTpl> _tpl;
    std::list<Ref<Var>> _param_vars;
    cls::Ancestry _ancestry;
    std::map<type_id_t, Ref<Fun>> _convs;
};

} // namespace ulam
