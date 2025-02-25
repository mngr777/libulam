#include <libulam/ast/nodes/expr.hpp>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/semantic/module.hpp>
#include <libulam/semantic/program.hpp>
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

Ref<Var> ClassTpl::add_param(Ref<ast::TypeName> type_node, Ref<ast::VarDecl> node) {
    auto var = ClassBase::add_param(type_node, node);
    var->set_flag(Var::Tpl);
    return var;
}

Ref<Var>
ClassTpl::add_const(Ref<ast::TypeName> type_node, Ref<ast::VarDecl> node) {
    auto var = ClassBase::add_const(type_node, node);
    var->set_flag(Var::Tpl);
    return var;
}

Ref<Prop>
ClassTpl::add_prop(Ref<ast::TypeName> type_node, Ref<ast::VarDecl> node) {
    auto prop = ClassBase::add_prop(type_node, node);
    prop->set_flag(Var::Tpl);
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
    {
        TypedValue tv;
        auto scope_view = param_scope()->view();
        scope_view->reset();
        while (true) {
            auto [name_id, sym] = scope_view->advance();
            if (!sym)
                break;

            assert(sym->is<Var>());
            auto var = sym->get<Var>();
            assert(var->is_const() && var->is(Var::ClassParam & Var::Tpl));

            std::swap(tv, args.front());
            args.pop_front();

            cls->add_param(var->type_node(), var->node(), tv.move_value());
        }
        assert(param_scope()->version() == scope_view->version()); // in sync?
        assert(args.size() == 0); // all args consumed?
    }

    // create members
    {
        auto scope_view = scope()->view();
        scope_view->reset();
        while (true) {
            auto [name_id_, sym] = scope_view->advance();
            if (!sym)
                break;

            // (cannot use struct. binding in lambda in C++17)
            auto name_id = name_id_; 
            sym->accept(
                [&](Ref<UserType> type) {
                    cls->add_type_def(type->as_alias()->node());
                },
                [&](Ref<Var> var) {
                    cls->add_const(var->type_node(), var->node());
                },
                [&](Ref<Prop> prop) {
                    cls->add_prop(prop->type_node(), prop->node());
                },
                [&](Ref<FunSet> fset) {
                    auto copy = make<FunSet>(*fset);
                    copy->for_each(
                        [&](Ref<Fun> fun) { fun->set_cls(ref(cls)); });
                    cls->scope()->set(name_id, ref(copy));
                    cls->set(name_id, std::move(copy));
                },
                [&](auto&&) { assert(false); });
        }
        assert(scope_view->version() == scope()->version()); // in sync?
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
