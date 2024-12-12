#include <cassert>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/ast/nodes/params.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type/class.hpp>
#include <string>

#define ULAM_DEBUG
#define ULAM_DEBUG_PREFIX "[Class] "
#include "src/debug.hpp"

namespace ulam {

// ClassBase

ClassBase::ClassBase(
    Ref<ast::ClassDef> node,
    Ref<Scope> module_scope,
    ScopeFlags scope_flags):
    _node{node},
    _param_scope{make<PersScope>(module_scope)},
    _inh_scope{make<PersScope>(ref(_param_scope))},
    _scope{make<PersScope>(ref(_inh_scope), scope_flags)} {
    assert(module_scope);
    assert(module_scope->is(scp::Module));
}

// Class

Class::Class(TypeIdGen& id_gen, Ref<ClassTpl> tpl):
    UserType{id_gen},
    ClassBase{tpl->node(), tpl->param_scope()->parent(), scp::Class},
    _tpl{tpl} {}

Class::Class(TypeIdGen& id_gen, Ref<ast::ClassDef> node, Ref<Scope> scope):
    UserType{id_gen}, ClassBase{node, scope, scp::Class}, _tpl{} {}

Class::~Class() {}

str_id_t Class::name_id() const { return node()->name().str_id(); }

// ClassTpl

ClassTpl::ClassTpl(
    TypeIdGen& id_gen, Ref<ast::ClassDef> node, Ref<Scope> scope):
    TypeTpl{id_gen}, ClassBase{node, scope, scp::ClassTpl} {}

ClassTpl::~ClassTpl() {}

str_id_t ClassTpl::name_id() const { return node()->name().str_id(); }

Ref<Type> ClassTpl::type(
    Diag& diag, Ref<ast::ArgList> args_node, TypedValueList&& args) {
    auto key = type_args_str(args);
    auto it = _types.find(key);
    if (it != _types.end())
        return ref(it->second);
    auto cls = inst(args_node, std::move(args));
    auto cls_ref = ref(cls);
    _types.emplace(key, std::move(cls));
    return cls_ref;
}

Ptr<Class>
ClassTpl::inst(Ref<ast::ArgList> args_node, TypedValueList&& args) {
    auto cls = make<Class>(id_gen(), this);

    // copy params
    {
        auto scope_proxy = param_scope()->proxy();
        scope_proxy.reset();
        while (true) {
            auto [name_id, sym] = scope_proxy.advance();
            if (!sym)
                break;
            assert(sym->is<Var>());
            auto var = sym->get<Var>();
            assert(var->is_const() && var->is(Var::ClassParam));
            TypedValue value;
            std::swap(value, args.front());
            args.pop_front();
            auto copy = make<Var>(
                var->type_node(), var->node(), std::move(value), var->flags());
            cls->param_scope()->set(name_id, ref(copy));
            cls->set(name_id, std::move(copy));
        }
        assert(param_scope()->version() == scope_proxy.version()); // in sync?
        assert(args.size() == 0); // all args consumed?
    }

    // copy members
    {
        auto scope_proxy = scope()->proxy();
        scope_proxy.reset();
        while (true) {
            auto [name_id, sym] = scope_proxy.advance();
            if (!sym)
                break;

            if (sym->is<UserType>()) {
                auto alias = sym->get<UserType>()->as_alias();
                assert(alias);
                Ptr<UserType> copy =
                    make<AliasType>(id_gen(), alias->node());
                cls->scope()->set(name_id, ref(copy));
                cls->set(name_id, std::move(copy));

            } else if (sym->is<Var>()) {
                auto var = sym->get<Var>();
                auto copy = make<Var>(
                    var->type_node(), var->node(), Ref<Type>{}, var->flags());
                cls->scope()->set(name_id, ref(copy));
                cls->set(name_id, std::move(copy));

            } else {
                assert(sym->is<Fun>());
                auto fun = sym->get<Fun>();
                auto copy = make<Fun>();
                for (auto& overload : fun->overloads()) {
                    copy->add_overload(
                        overload.ref()->node(), cls->scope()->state());
                }
                cls->scope()->set(name_id, ref(copy));
                cls->set(name_id, std::move(fun));
            }
        }
        assert(scope_proxy.version() == scope()->version()); // in sync?
    }
    return cls;
}

std::string ClassTpl::type_args_str(const TypedValueList& args) {
    // !! this is a temporary implementation
    std::string str;
    for (const auto& arg : args) {
        auto rval = arg.value().rvalue();
        assert(!rval->is_unknown());
        if (!str.empty())
            str += "_";
        if (rval->is<Integer>()) {
            str += std::to_string(rval->get<Integer>());
        } else if (rval->is<Unsigned>()) {
            str += std::to_string(rval->get<Unsigned>());
        } else if (rval->is<Bool>()) {
            str += (rval->get<Bool>() ? "t" : "f");
        } else if (rval->is<String>()) {
            // str += std::to_string(
            //     program()->ast()->ctx().str_id(rval->get<String>()));
            return rval->get<String>();
        } else {
            assert(false);
        }
    }
    return str;
}

} // namespace ulam
