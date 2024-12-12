#include <cassert>
#include <libulam/diag.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/resolve_deps.hpp>
#include <libulam/sema/visitor.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/class_kind.hpp>
#include <libulam/semantic/type_tpl.hpp>
#include <libulam/semantic/var.hpp>
#include <libulam/str_pool.hpp>
#include <unordered_map>
#include <unordered_set>

#define ULAM_DEBUG
#define ULAM_DEBUG_PREFIX "[sema::ResolveDeps] "
#include "src/debug.hpp"

namespace ulam::sema {

void ResolveDeps::visit(Ref<ast::Root> node) {
    assert(!node->program());
    // make program
    node->set_program(make<Program>(diag(), node));
    RecVisitor::visit(node);
    export_classes();
}

void ResolveDeps::visit(Ref<ast::ModuleDef> node) {
    assert(!node->module());
    auto module = program()->add_module(node);
    node->set_module(module);
    RecVisitor::visit(node);
}

bool ResolveDeps::do_visit(Ref<ast::ClassDef> node) {
    assert(pass() == Pass::Module);
    assert(!node->type() && !node->type_tpl());
    auto name_id = node->name().str_id();
    // already defined?
    auto prev = module()->get(name_id);
    if (prev) {
        diag().emit(
            diag::Error, node->name().loc_id(), str(name_id).size(),
            "already defined"); // TODO: say where
        return false;
    }

    // add to module, set node attrs
    auto scope_proxy = scopes().top<PersScopeProxy>();
    if (node->params()) {
        assert(node->params()->child_num() > 0);
        assert(node->kind() != ClassKind::Element);

        // class tpl
        auto params = node->params();
        auto tpl =
            make<ClassTpl>(program()->type_id_gen(), node, module()->scope());
        auto tpl_ref = ref(tpl);

        // add params
        for (unsigned n = 0; n < params->child_num(); ++n) {
            auto param_node = params->get(n);
            auto param_name_id = param_node->name().str_id();
            // make var
            auto var = make<Var>(
                param_node->type_name(), param_node, Ref<Type>{},
                Var::ClassParam | Var::IsConst);
            auto var_ref = ref(var);
            tpl->set(param_name_id, std::move(var)); // set class tpl var
            tpl->param_scope()->set(param_name_id, var_ref); // add to tpl scope
            param_node->set_scope_proxy(
                tpl->param_scope()->proxy()); // set node scope attr
        }
        module()->set<ClassTpl>(name_id, std::move(tpl)); // add to module
        scope_proxy->set(name_id, tpl_ref);               // add to scope
        node->set_scope_proxy(*scope_proxy);              // set node scope attr
        node->set_type_tpl(tpl_ref);                      // set node attr

    } else {
        // class
        auto cls =
            make<Class>(program()->type_id_gen(), node, module()->scope());
        auto cls_ref = ref(cls);
        module()->set<Class>(name_id, std::move(cls)); // add to module
        scope_proxy->set(name_id, cls_ref);            // add to scope
        node->set_scope_proxy(*scope_proxy);           // set node scope attr
        node->set_type(cls_ref);                       // set node attr
    }
    return true;
}

void ResolveDeps::visit(Ref<ast::TypeDef> node) {
    // don't skip the type name
    visit(node->type_name());

    auto alias_id = node->alias_id();
    // name is already in current scope?
    if (scope()->has(alias_id, true)) {
        // TODO: after types are resolves, complain if types don't match
        return;
    }

    auto class_node = class_def();
    if (scope()->is(scp::Persistent)) {
        // persistent typedef (module-local or class/class tpl)
        Ref<UserType> type_ref{};
        auto scope_proxy = scopes().top<PersScopeProxy>();
        if (scope_proxy->is(scp::Module)) {
            // module typedef (is not a module member)
            Ptr<UserType> type =
                make<AliasType>(program()->type_id_gen(), node);
            type_ref = ref(type);
            scope_proxy->set(alias_id, std::move(type)); // add to scope

        } else if (scope_proxy->is(scp::Class)) {
            // class member
            assert(class_node->type());
            auto cls = class_node->type();
            Ptr<UserType> type =
                make<AliasType>(program()->type_id_gen(), node);
            type_ref = ref(type);
            cls->set(alias_id, std::move(type));  // add to class
            scope_proxy->set(alias_id, type_ref); // add to scope

        } else if (scope_proxy->is(scp::ClassTpl)) {
            // class tpl member
            assert(class_node->type_tpl());
            auto tpl = class_node->type_tpl();
            Ptr<UserType> type =
                make<AliasType>(program()->type_id_gen(), node);
            type_ref = ref(type);
            tpl->set(alias_id, std::move(type));  // add to class tpl
            scope_proxy->set(alias_id, type_ref); // add to scope
        } else {
            assert(false);
        }
        node->set_scope_proxy(*scope_proxy); // set node scope proxy
        node->set_alias_type(type_ref->as_alias());

    } else {
        // transient typedef (in function body)
        assert(scope()->in(scp::Fun));
        Ptr<UserType> type = make<AliasType>(program()->type_id_gen(), node);
        scope()->set(alias_id, std::move(type));
    }
}

void ResolveDeps::visit(Ref<ast::VarDefList> node) {
    // don't skip the type name
    visit(node->type_name());

    if (!scope()->is(scp::Persistent))
        return;

    // create and set module/class/tpl variables
    auto class_node = class_def();
    auto scope_proxy = scopes().top<PersScopeProxy>();
    for (unsigned n = 0; n < node->def_num(); ++n) {
        auto def = node->def(n);
        auto name_id = def->name().str_id();

        // already in current scope?
        if (scope_proxy->has(name_id, true)) {
            diag().emit(diag::Error, def->loc_id(), 1, "already defined");
            continue;
        }

        // make
        Var::Flag flags = node->is_const() ? Var::IsConst : Var::NoFlags;
        auto var = make<Var>(node->type_name(), def, Ref<Type>{}, flags);
        auto var_ref = ref(var);
        // install
        if (class_node) {
            // class/tpl variable
            if (class_node->type()) {
                class_node->type()->set(name_id, std::move(var));
            } else {
                assert(class_node->type_tpl());
                class_node->type_tpl()->set(name_id, std::move(var));
            }
            scope_proxy->set(name_id, var_ref); // add to scope
            def->set_scope_proxy(*scope_proxy); // set node scope proxy
        } else {
            // module constant (is not a module member)
            scope_proxy->set(name_id, std::move(var));
            def->set_scope_proxy(*scope_proxy); // set node scope proxy
        }
        def->set_var(var_ref); // set node attr
    }
    return;
}

void ResolveDeps::visit(Ref<ast::FunDef> node) {
    // don't skip ret type and params
    visit(node->ret_type_name());
    visit(node->params());

    // get class/tpl, name
    auto class_node = class_def();
    auto name_id = NoStrId;
    Ref<ClassBase> cls_base{};
    if (class_node->type()) {
        cls_base = class_node->type();
        name_id = class_node->type()->name_id();
    } else {
        cls_base = class_node->type_tpl();
        name_id = class_node->type_tpl()->name_id();
    }
    assert(cls_base);
    assert(name_id != NoStrId);

    // find or create fun
    auto sym = cls_base->get(name_id);
    Ref<Fun> fun_ref{};
    if (sym) {
        if (!sym->is<Fun>()) {
            diag().emit(
                diag::Error, node->loc_id(), str(name_id).size(),
                "defined and is not a function");
            return;
        }
    } else {
        sym = cls_base->set(name_id, make<Fun>());        // add to class/tpl
        cls_base->scope()->set(name_id, sym->get<Fun>()); // add to scope
    }
    fun_ref = sym->get<Fun>();

    // create overload
    auto overload = fun_ref->add_overload(node, cls_base->scope()->state());
    node->set_overload(overload);
    return;
}

bool ResolveDeps::do_visit(Ref<ast::TypeName> node) {
    // set type/tpl TypeSpec attr
    // NOTE: any name not in scope has to be imported and
    // imported names can be later unambigously resolved without scope
    auto type_spec = node->first();
    if (type_spec->is_builtin()) {
        // builtin type/tpl
        BuiltinTypeId id = type_spec->builtin_type_id();
        if (has_bitsize(id)) {
            type_spec->set_type_tpl(program()->builtins().prim_type_tpl(id));
        } else if (is_prim(id)) {
            type_spec->set_type(program()->builtins().prim_type(id));
        }
        return false;
    }
    auto name_id = type_spec->ident()->name().str_id();
    if (scope()->has(name_id)) {
        // set type/tpl
        auto sym = scope()->get(name_id);
        if (sym->is<UserType>()) {
            type_spec->set_type(sym->get<UserType>());
        } else {
            assert(sym->is<ClassTpl>());
            type_spec->set_cls_tpl(sym->get<ClassTpl>());
        }
    } else {
        // add external dependency
        module()->add_dep(name_id);
        // postpone until all exports are known (store module ID to avoid
        // importing from itself)
        _unresolved.push_back({module()->id(), node});
    }
    return false;
}

void ResolveDeps::export_classes() {
    // collect set of exporting modules for each exported symbol
    // (must be 1 per symbol)
    using ModuleSet = std::unordered_set<Ref<Module>>;
    std::unordered_map<str_id_t, ModuleSet> exporting;
    for (auto& mod : program()->modules()) {
        for (auto& pair : *mod) {
            auto& [name_id, sym] = pair;
            auto [it, inserted] = exporting.emplace(name_id, ModuleSet{});
            assert(sym.is<Class>() || sym.is<ClassTpl>());
            it->second.insert(ref(mod));
        }
    }
    // remove names defined in multiple modules
    for (auto it = exporting.begin(); it != exporting.end();) {
        auto name_id = it->first;
        auto& mods = it->second;
        if (mods.size() > 1) {
            diag().emit(
                diag::Error, std::string{"multiple modules define"} +
                                 std::string{str(name_id)} + ", ignoring");
            it = exporting.erase(it);
        } else {
            ++it;
        }
    }
    // import dependencies
    for (auto& mod : program()->modules()) {
        for (auto name_id : mod->deps()) {
            auto it = exporting.find(name_id);
            if (it == exporting.end())
                continue;
            assert(it->second.size() == 1);
            auto& exporter = *it->second.begin();
            if (*exporter != *mod) {
                auto sym = exporter->get(name_id);
                assert(sym);
                if (sym->is<Class>()) {
                    mod->add_import(name_id, exporter, sym->get<Class>());
                } else {
                    assert(sym->is<ClassTpl>());
                    mod->add_import(name_id, exporter, sym->get<ClassTpl>());
                }
            }
        }
    }
    // resolve (first ident of) posponed TypeName's
    for (auto item : _unresolved) {
        assert(!item.node->first()->is_builtin());
        auto type_spec = item.node->first();
        auto name_id = type_spec->ident()->name().str_id();
        auto it = exporting.find(name_id);
        if (it != exporting.end()) {
            auto& exporter = *it->second.begin();
            auto sym = exporter->get(name_id);
            assert(sym);
            // set type/tpl
            // don't use symbols from same module
            if (exporter->id() != item.module_id) {
                if (sym->is<Class>()) {
                    type_spec->set_type(sym->get<Class>());
                } else {
                    assert(sym->is<ClassTpl>());
                    type_spec->set_type_tpl(sym->get<ClassTpl>());
                }
                continue; // success
            }
        }
        diag().emit(
            diag::Error, item.node->loc_id(), str(name_id).size(),
            "failed to resolve");
    }
}

} // namespace ulam::sema
