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
    node->set_program(make<Program>(diag(), node->ctx().str_pool()));
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
    assert(!node->cls() && !node->cls_tpl());
    assert(scope()->is(scp::Module));

    auto name_id = node->name().str_id();

    // already defined?
    auto prev = module()->get(name_id);
    if (prev) {
        diag().emit(
            Diag::Error, node->name().loc_id(), str(name_id).size(),
            "already defined"); // TODO: say where
        return false;
    }

    // add to module, set node attrs
    if (node->params()) {
        assert(node->params()->child_num() > 0);
        assert(node->kind() != ClassKind::Element);

        // class tpl
        auto params = node->params();
        auto tpl = make<ClassTpl>(
            program()->type_id_gen(), ast()->ctx().str_pool(), node,
            module()->scope());
        auto tpl_ref = ref(tpl);

        // add tpl params
        for (unsigned n = 0; n < params->child_num(); ++n) {
            auto param_node = params->get(n);
            auto param_name_id = param_node->name().str_id();
            // make var
            auto var = make<Var>(
                param_node->type_name(), param_node, Ref<Type>{},
                Var::Tpl | Var::ClassParam | Var::Const);
            auto var_ref = ref(var);
            // set tpl var symbol, add to scope, store scope version
            tpl->set(param_name_id, std::move(var));
            param_node->set_scope_version(tpl->param_scope()->version());
            tpl->param_scope()->set(param_name_id, var_ref);
        }
        // set module tpl symbol, add to scope, store scope version
        module()->set<ClassTpl>(name_id, std::move(tpl));
        scope()->set(name_id, tpl_ref);
        node->set_scope_version(scope()->version());
        node->set_cls_tpl(tpl_ref); // link to node

    } else {
        // class
        auto name = program()->str_pool().get(name_id);
        auto cls = make<Class>(
            &program()->type_id_gen(), name, node, module()->scope());
        auto cls_ref = ref(cls);
        // set module class symbol, add to scope, store scope version
        node->set_scope_version(scope()->version());
        module()->set<Class>(name_id, std::move(cls));
        scope()->set(name_id, cls_ref);
        node->set_cls(cls_ref); // link to node
    }
    return true;
}

void ResolveDeps::visit(Ref<ast::TypeDef> node) {
    // visit TypeName
    assert(node->has_type_name());
    node->type_name()->accept(*this);

    auto alias_id = node->alias_id();
    // name is already in current scope?
    if (scope()->has(alias_id, true)) {
        // TODO: after types are resolves, complain if types don't match
        return;
    }

    auto class_node = class_def();
    if (scope()->is(scp::Persistent)) {
        // persistent typedef (module-local or class/tpl)
        Ptr<UserType> type = make<AliasType>(&program()->type_id_gen(), node);
        auto type_ref = ref(type);
        auto scope_version = scope()->version();
        if (scope()->is(scp::Module)) {
            // module typedef (is not a module member)
            scope()->set(alias_id, std::move(type));

        } else if (scope()->is(scp::Class)) {
            // class member
            assert(class_node->cls());
            auto cls = class_node->cls();
            // add to class, add to scope
            cls->set(alias_id, std::move(type));
            scope()->set(alias_id, type_ref);

        } else if (scope()->is(scp::ClassTpl)) {
            // tpl member
            assert(class_node->cls_tpl());
            auto tpl = class_node->cls_tpl();
            // add to class tpl, add to scope
            tpl->set(alias_id, std::move(type));
            scope()->set(alias_id, type_ref);
        } else {
            assert(false);
        }
        // store scope version and alias type
        node->set_scope_version(scope_version);
        node->set_alias_type(type_ref->as_alias());

    } else {
        // transient typedef (in function body)
        Ptr<UserType> type = make<AliasType>(nullptr, node);
        scope()->set(alias_id, std::move(type));
    }
}

void ResolveDeps::visit(Ref<ast::VarDefList> node) {
    // visit TypeName
    assert(node->has_type_name());
    node->type_name()->accept(*this);

    if (!scope()->is(scp::Persistent))
        return;

    // create and set module/class/tpl variables
    auto class_node = class_def();
    for (unsigned n = 0; n < node->def_num(); ++n) {
        auto def = node->def(n);
        auto name_id = def->name().str_id();

        // already in current scope?
        if (scope()->has(name_id, true)) {
            diag().emit(Diag::Error, def->loc_id(), 1, "already defined");
            continue;
        }

        // make
        Var::Flag flags = Var::NoFlags;
        if (node->is_const())
            flags |= Var::Const;
        if (class_node->cls_tpl())
            flags |= Var::Tpl;
        auto var = make<Var>(node->type_name(), def, Ref<Type>{}, flags);
        auto var_ref = ref(var);

        // install
        auto scope_version = scope()->version();
        if (class_node) {
            // class/tpl variable
            if (class_node->cls()) {
                class_node->cls()->set(name_id, std::move(var));
            } else {
                assert(class_node->cls_tpl());
                class_node->cls_tpl()->set(name_id, std::move(var));
            }
            // add to scope
            scope()->set(name_id, var_ref);
        } else {
            // module constant (is not a module member)
            scope()->set(name_id, std::move(var));
        }
        def->set_scope_version(scope_version); // store scope version
        def->set_var(var_ref);                 // set node attr
    }
}

bool ResolveDeps::do_visit(Ref<ast::FunDef> node) {
    assert(scope()->is(scp::Class) || scope()->is(scp::ClassTpl));

    // get class/tpl, name
    auto class_node = class_def();
    Ref<ClassBase> cls_base{};
    if (class_node->cls()) {
        cls_base = class_node->cls();
    } else {
        cls_base = class_node->cls_tpl();
    }
    assert(cls_base);

    // find or create fun set
    auto name_id = node->name().str_id();
    auto scope_version = scope()->version();
    auto sym = cls_base->get(name_id);
    Ref<FunSet> fset{};
    if (sym) {
        if (!sym->is<FunSet>()) {
            diag().emit(
                Diag::Error, node->loc_id(), str(name_id).size(),
                "defined and is not a function");
            return false;
        }
    } else {
        // add to class/tpl, add to scope
        sym = cls_base->set(name_id, make<FunSet>());
        scope()->set(name_id, sym->get<FunSet>());
    }
    fset = sym->get<FunSet>();

    // create overload
    auto fun = fset->add(node, scope_version);
    node->set_fun(fun);
    node->set_scope_version(scope_version);
    return true;
}

// TODO: do not set type/tpl, do it in resolver
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
            auto type = sym->get<UserType>();
            if (type->has_id()) // avoid setting fun-local aliases
                type_spec->set_type(type);
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
            for (auto& mod : mods) {
                auto sym = mod->get(name_id);
                assert(sym);
                Ref<ClassBase> cls_base{};
                if (sym->is<Class>()) {
                    cls_base = sym->get<Class>();
                } else {
                    assert(sym->is<ClassTpl>());
                    cls_base = sym->get<ClassTpl>();
                }
                diag().emit(
                    Diag::Error, cls_base->node()->loc_id(),
                    str(name_id).size(), "defined in multiple modules");
            }
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
            // don't use symbols from same module (disabled)
            if (true || exporter->id() != item.module_id) {
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
            Diag::Error, item.node->loc_id(), str(name_id).size(),
            "failed to resolve");
    }
}

} // namespace ulam::sema
