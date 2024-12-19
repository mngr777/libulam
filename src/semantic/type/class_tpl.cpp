#include <libulam/ast/nodes/expr.hpp>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/class_tpl.hpp>
#include <libulam/semantic/scope/view.hpp>
#include <utility>

namespace ulam {

ClassTpl::ClassTpl(
    TypeIdGen& id_gen,
    UniqStrPool& str_pool,
    Ref<ast::ClassDef> node,
    Ref<Scope> scope):
    TypeTpl{id_gen},
    ClassBase{node, scope, scp::ClassTpl},
    _str_pool{str_pool} {}

ClassTpl::~ClassTpl() {}

str_id_t ClassTpl::name_id() const { return node()->name().str_id(); }

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
    auto cls = make<Class>(&id_gen(), this);

    // create params
    {
        auto scope_view = param_scope()->view();
        scope_view->reset();
        while (true) {
            auto [name_id, sym] = scope_view->advance();
            if (!sym)
                break;
            assert(sym->is<Var>());
            auto var = sym->get<Var>();
            assert(var->is_const() && var->is(Var::ClassParam & Var::Tpl));
            TypedValue value;
            std::swap(value, args.front());
            args.pop_front();
            auto copy = make<Var>(
                var->type_node(), var->node(), std::move(value),
                var->flags() & ~Var::Tpl);
            cls->param_scope()->set(name_id, ref(copy));
            cls->set(name_id, std::move(copy));
        }
        assert(param_scope()->version() == scope_view->version()); // in sync?
        assert(args.size() == 0); // all args consumed?
    }

    // create members
    {
        auto scope_view = scope()->view();
        scope_view->reset();
        while (true) {
            auto [name_id, sym] = scope_view->advance();
            if (!sym)
                break;

            if (sym->is<UserType>()) {
                auto alias = sym->get<UserType>()->as_alias();
                assert(alias);
                Ptr<UserType> copy = make<AliasType>(&id_gen(), alias->node());
                cls->scope()->set(name_id, ref(copy));
                cls->set(name_id, std::move(copy));

            } else if (sym->is<Var>()) {
                auto var = sym->get<Var>();
                auto copy = make<Var>(
                    var->type_node(), var->node(), Ref<Type>{},
                    var->flags() & ~Var::Tpl);
                cls->scope()->set(name_id, ref(copy));
                cls->set(name_id, std::move(copy));

            } else {
                assert(sym->is<FunSet>());
                auto fun = sym->get<FunSet>();
                auto copy = make<FunSet>();
                for (auto& overload : fun->overloads()) {
                    copy->add_overload(
                        overload.ref()->node(), cls->scope()->version());
                }
                cls->scope()->set(name_id, ref(copy));
                cls->set(name_id, std::move(fun));
            }
        }
        assert(scope_view->version() == scope()->version()); // in sync?
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
