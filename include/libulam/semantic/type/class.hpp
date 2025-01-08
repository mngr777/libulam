#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/decl.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/class/ancestry.hpp>
#include <libulam/semantic/type/class/base.hpp>
#include <string_view>

namespace ulam::ast {
class ArgList;
class ClassDef;
class TypeName;
} // namespace ulam::ast

namespace ulam {

class Diag;
class ClassTpl;

class Class : public UserType, public ClassBase {
    friend ClassTpl;

public:
    using ParamVarList = std::list<Ref<Var>>;

    Class(TypeIdGen* id_gen, std::string_view name, Ref<ClassTpl> tpl);
    Class(
        TypeIdGen* id_gen,
        std::string_view name,
        Ref<ast::ClassDef> node,
        Ref<Scope> scope);
    ~Class();

    const std::string_view name() const { return _name; }
    str_id_t name_id() const override;

    const ParamVarList& param_vars() const { return _param_vars; }

    void add_ancestor(Ref<Class> cls, Ref<ast::TypeName> node);

    auto& parents() { return _ancestry.parents(); }
    const auto& parents() const { return _ancestry.parents(); }

    bitsize_t bitsize() const override;

    Ref<Class> as_class() override { return this; }
    Ref<const Class> as_class() const override { return this; }

private:
    void add_param_var(Ptr<Var>&& var);

    std::string_view _name;
    Ref<ClassTpl> _tpl;
    std::list<Ref<Var>> _param_vars;
    cls::Ancestry _ancestry;
};

} // namespace ulam
