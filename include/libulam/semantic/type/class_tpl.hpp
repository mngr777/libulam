#pragma once
#include <libulam/detail/variant.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/decl.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/class/base.hpp>
#include <libulam/semantic/type_tpl.hpp>
#include <libulam/str_pool.hpp>
#include <list>
#include <string>
#include <unordered_map>

namespace ulam::ast {
class ArgList;
class ClassDef;
class Param;
class TypeName;
class VarDecl;
} // namespace ulam::ast

namespace ulam::sema {
class Resolver;
}

namespace ulam {

class Class;
class Module;
class Scope;

class ClassTpl : public TypeTpl, public ClassBase, public Decl {
    friend Class;

public:
    ClassTpl(Ref<ast::ClassDef> node, Ref<Module> module);
    ~ClassTpl();

    str_id_t name_id() const;

    using ClassBase::add_param;

    Ref<Var>
    add_param(Ref<ast::TypeName> type_node, Ref<ast::VarDecl> node) override;

    Ref<AliasType> add_type_def(Ref<ast::TypeDef> node) override;

    Ref<Fun> add_fun(Ref<ast::FunDef> node) override;

    Ref<Var>
    add_const(Ref<ast::TypeName> type_node, Ref<ast::VarDecl> node) override;

    Ref<Prop>
    add_prop(Ref<ast::TypeName> type_node, Ref<ast::VarDecl> node) override;

    Ref<Class> type(TypedValueList&& args);

private:
    using Member = detail::RefVariant<AliasType, Var, Prop, Fun>;

    Ptr<Class> inst(TypedValueList&& args);

    std::string type_args_str(const TypedValueList& args);

    Ref<ast::ClassDef> _node;
    std::unordered_map<std::string, Ptr<Class>> _classes;
    std::list<Member> _ordered_members;
};

} // namespace ulam
