#include <libulam/ast/nodes/expr.hpp>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/semantic/module.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope/iterator.hpp>
#include <libulam/semantic/scope/view.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/class_tpl.hpp>
#include <utility>

namespace ulam {

ClassTpl::ClassTpl(Ref<ast::ClassDef> node, Ref<Module> module):
    TypeTpl{module->program()->type_id_gen()},
    ClassBase{node, module, scp::ClassTpl} {}

ClassTpl::~ClassTpl() {}

str_id_t ClassTpl::name_id() const { return node()->name().str_id(); }

Ref<Var>
ClassTpl::add_param(Ref<ast::TypeName> type_node, Ref<ast::VarDecl> node) {
    auto var = ClassBase::add_param(type_node, node);
    var->set_flag(Var::Tpl);
    return var;
}

Ref<AliasType> ClassTpl::add_type_def(Ref<ast::TypeDef> node) {
    auto alias = ClassBase::add_type_def(node);
    _ordered_members.emplace_back(alias);
    return alias;
}

Ref<Fun> ClassTpl::add_fun(Ref<ast::FunDef> node) {
    auto fun = ClassBase::add_fun(node);
    _ordered_members.emplace_back(fun);
    return fun;
}

Ref<Var>
ClassTpl::add_const(Ref<ast::TypeName> type_node, Ref<ast::VarDecl> node) {
    auto var = ClassBase::add_const(type_node, node);
    var->set_flag(Var::Tpl);
    _ordered_members.emplace_back(var);
    return var;
}

Ref<Prop>
ClassTpl::add_prop(Ref<ast::TypeName> type_node, Ref<ast::VarDecl> node) {
    auto prop = ClassBase::add_prop(type_node, node);
    prop->set_flag(Var::Tpl);
    _ordered_members.emplace_back(prop);
    return prop;
}

Ref<Type>
ClassTpl::type(Diag& diag, Ref<ast::ArgList> args_node, TypedValueList&& args) {
    auto key = type_args_str(args);
    auto it = _classes.find(key);
    if (it != _classes.end())
        return ref(it->second);
    auto cls = inst(args_node, std::move(args));
    auto cls_ref = ref(cls);
    _classes.emplace(key, std::move(cls));
    return cls_ref;
}

Ptr<Class> ClassTpl::inst(Ref<ast::ArgList> args_node, TypedValueList&& args) {
    auto& str_pool = module()->program()->str_pool();
    auto cls = make<Class>(str_pool.get(name_id()), this);

    // create params
    TypedValue tv;
    for (auto var : params()) {
        assert(args.size() > 0);
        std::swap(tv, args.front());
        args.pop_front();

        // assert(tv.type()->is_same(var->type())); // TODO
        cls->add_param(var->type_node(), var->node(), tv.move_value());
    }

    // create other members
    for (auto& mem : _ordered_members) {
        mem.accept(
            [&](Ref<AliasType> alias) { cls->add_type_def(alias->node()); },
            [&](Ref<Var> var) {
                cls->add_const(var->type_node(), var->node());
            },
            [&](Ref<Prop> prop) {
                cls->add_prop(prop->type_node(), prop->node());
            },
            [&](Ref<Fun> fun) { cls->add_fun(fun->node()); },
            [&](auto) { assert(false); });
    }
    return cls;
}

std::string ClassTpl::type_args_str(const TypedValueList& args) {
    // TMP
    std::string str;
    for (auto& arg : args) {
        // TODO: visit RValue instead to avoid copying
        auto rval = arg.value().copy_rvalue();
        assert(!rval.empty());
        if (!str.empty())
            str += "_";
        if (rval.is<Integer>()) {
            str += std::to_string(rval.get<Integer>());
        } else if (rval.is<Unsigned>()) {
            str += std::to_string(rval.get<Unsigned>());
            // } else if (rval.is<Bool>()) {
            //     str += (rval.get<Bool>() ? "t" : "f");
        } else if (rval.is<String>()) {
            // str += std::to_string(
            //     program()->ast()->ctx().str_id(rval.get<String>()));
            return rval.get<String>();
        } else {
            assert(false);
        }
    }
    return str;
}

} // namespace ulam
