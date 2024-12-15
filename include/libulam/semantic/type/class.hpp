#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/scope/object.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/class/base.hpp>
#include <libulam/semantic/type/class/layout.hpp>

namespace ulam::ast {
class ArgList;
class ClassDef;
class TypeName;
} // namespace ulam::ast

namespace ulam {

class Diag;
class ClassTpl;

class Class : public UserType, public ClassBase {
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

    Class(TypeIdGen* id_gen, Ref<ClassTpl> tpl);
    Class(TypeIdGen* id_gen, Ref<ast::ClassDef> node, Ref<Scope> scope);
    ~Class();

    str_id_t name_id() const override;

    // TMP
    auto& ancestors() { return _ancestors; }
    const auto& ancestors() const { return _ancestors; }

    void add_ancestor(Ref<Class> anc, Ref<ast::TypeName> node) {
        _ancestors.emplace_back(anc, node);
    }

    bitsize_t bitsize() const override;

    Ref<Class> as_class() override { return this; }
    Ref<const Class> as_class() const override { return this; }

private:
    Ref<ClassTpl> _tpl;
    std::list<Ancestor> _ancestors;
};

} // namespace ulam
