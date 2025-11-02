#include <libulam/ast/nodes/expr.hpp>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/sema/resolver.hpp>
#include <libulam/semantic/mangler.hpp>
#include <libulam/semantic/module.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/class_tpl.hpp>
#include <utility>

namespace ulam {

ClassTpl::ClassTpl(
    const std::string_view name, Ref<ast::ClassDef> node, Ref<Module> module):
    TypeTpl{module->program()->type_id_gen()},
    ClassBase{node, module, nullptr, scp::ClassTpl},
    _name{name} {
    set_module(module);
}

ClassTpl::~ClassTpl() {}

str_id_t ClassTpl::name_id() const { return node()->name().str_id(); }

Ref<Var> ClassTpl::add_param(Ref<ast::Param> node) {
    return add_param(node->type_name(), node);
}

Ref<Var>
ClassTpl::add_param(Ref<ast::TypeName> type_node, Ref<ast::VarDecl> node) {
    auto var =
        make<Var>(type_node, node, Ref<Type>{}, Var::ClassParam | Var::Const);
    auto ref = ClassBase::add_param(std::move(var));
    ref->set_flag(Var::Tpl | Var::ClassParam);
    return ref;
}

Ref<AliasType> ClassTpl::add_type_def(Ref<ast::TypeDef> node) {
    auto alias = ClassBase::add_type_def(node);
    alias->set_cls_tpl(this);
    _ordered_members.emplace_back(alias);
    return alias;
}

Ref<Fun> ClassTpl::add_fun(Ref<ast::FunDef> node) {
    auto fun = ClassBase::add_fun(node);
    fun->set_cls_tpl(this);
    _ordered_members.emplace_back(fun);
    return fun;
}

Ref<Var>
ClassTpl::add_const(Ref<ast::TypeName> type_node, Ref<ast::VarDecl> node) {
    auto var = ClassBase::add_const(type_node, node);
    var->set_cls_tpl(this);
    var->set_flag(Var::Tpl);
    _ordered_members.emplace_back(var);
    return var;
}

Ref<Prop>
ClassTpl::add_prop(Ref<ast::TypeName> type_node, Ref<ast::VarDecl> node) {
    auto prop = ClassBase::add_prop(type_node, node);
    prop->set_cls_tpl(this);
    prop->set_flag(Var::Tpl);
    _ordered_members.emplace_back(prop);
    return prop;
}

Ref<Class> ClassTpl::type(TypedValueList&& args) {
    auto key = type_args_str(args);
    auto it = _classes.find(key);
    if (it != _classes.end())
        return ref(it->second);
    auto cls = inst(std::move(args));
    auto cls_ref = ref(cls);
    _classes.emplace(key, std::move(cls));
    return cls_ref;
}

Ptr<Class> ClassTpl::inst(TypedValueList&& args) {
    auto& str_pool = program()->str_pool();
    auto cls = make<Class>(str_pool.get(name_id()), this);

    // create params
    TypedValue tv;
    for (auto tpl_param : params()) {
        assert(args.size() > 0);
        std::swap(tv, args.front());
        args.pop_front();

        auto param = make<Var>(
            tpl_param->type_node(), tpl_param->node(), tv.type(),
            tv.move_value(), Var::ClassParam | Var::Const);
        cls->add_param(std::move(param));
    }

    // create other members
    for (auto& mem : _ordered_members) {
        auto decl = mem.accept(
            [&](Ref<AliasType> alias) -> Ref<Decl> {
                return cls->add_type_def(alias->node());
            },
            [&](Ref<Var> var) -> Ref<Decl> {
                return cls->add_const(var->type_node(), var->node());
            },
            [&](Ref<Prop> prop) -> Ref<Decl> {
                return cls->add_prop(prop->type_node(), prop->node());
            },
            [&](Ref<Fun> fun) -> Ref<Decl> {
                return cls->add_fun(fun->node());
            },
            [&](auto) -> Ref<Decl> { assert(false); });
        decl->set_cls_tpl(this);
    }
    return cls;
}

std::string ClassTpl::type_args_str(const TypedValueList& args) {
    return program()->mangler().mangled(args);
}

Ref<Program> ClassTpl::program() { return module()->program(); }

} // namespace ulam
