#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/scope/object.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/class/base.hpp>
#include <libulam/semantic/type/class/layout.hpp>
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
    class Ancestor {
    public:
        Ancestor(Ref<Class> cls, Ref<ast::TypeName> node):
            _cls{cls}, _node{node} {}

        Ref<Class> cls() { return _cls; }
        Ref<const Class> cls() const { return _cls; }

        Ref<ast::TypeName> node() { return _node; }
        Ref<const ast::TypeName> node() const { return _node; }

    private:
        Ref<Class> _cls;
        Ref<ast::TypeName> _node;
    };

    using ParamVarList = std::list<Ref<Var>>;

    Class(TypeIdGen* id_gen, std::string_view name, Ref<ClassTpl> tpl);
    Class(TypeIdGen* id_gen, std::string_view name, Ref<ast::ClassDef> node, Ref<Scope> scope);
    ~Class();

    const std::string_view name() const { return _name; }
    str_id_t name_id() const override;

    const ParamVarList& param_vars() const { return _param_vars; }

    auto& ancestors() { return _ancestors; } // TMP
    const auto& ancestors() const { return _ancestors; }

    void add_ancestor(Ref<Class> anc, Ref<ast::TypeName> node) {
        _ancestors.emplace_back(anc, node);
    }

    bitsize_t bitsize() const override;

    Ref<Class> as_class() override { return this; }
    Ref<const Class> as_class() const override { return this; }

private:
    void add_param_var(Ptr<Var>&& var);

    Ref<cls::Layout> layout();
    void init_layout();

    std::string_view _name;
    Ref<ClassTpl> _tpl;
    std::list<Ref<Var>> _param_vars;
    std::list<Ancestor> _ancestors;
    Ptr<cls::Layout> _layout;
};

} // namespace ulam
