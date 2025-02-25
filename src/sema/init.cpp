#include <cassert>
#include <libulam/diag.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/init.hpp>
#include <libulam/sema/visitor.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/class/prop.hpp>
#include <libulam/semantic/type/class_kind.hpp>
#include <libulam/semantic/type_tpl.hpp>
#include <libulam/semantic/var.hpp>
#include <libulam/str_pool.hpp>
#include <unordered_map>
#include <unordered_set>

#define DEBUG_INIT // TEST
#ifdef DEBUG_INIT
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[sema::Init] "
#    include "src/debug.hpp"
#endif

namespace ulam::sema {

void Init::visit(Ref<ast::Root> node) {
    assert(!node->program());
    node->set_program(make<Program>(diag(), node->ctx().str_pool()));
    RecVisitor::visit(node);
    export_classes();
}

void Init::visit(Ref<ast::ModuleDef> node) {
    assert(!node->module());
    auto module = program()->add_module(node);
    node->set_module(module);
    RecVisitor::visit(node);
}

bool Init::do_visit(Ref<ast::ClassDef> node) {
    assert(pass() == Pass::Module);
    assert(!node->cls() && !node->cls_tpl());
    assert(scope()->is(scp::Module));

    auto name_id = node->name().str_id();

    // already defined?
    auto prev = module()->get(name_id);
    if (prev) {
        diag().error(
            node->name().loc_id(), str(name_id).size(),
            "already defined"); // TODO: say where
        return false;
    }

    module()->add_class_or_tpl(node);
    sync_scope(node);
    return true;
}

void Init::visit(Ref<ast::TypeDef> node) {
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
        if (scope()->is(scp::Module)) {
            module()->add_type_def(node);
        } else if (scope()->is(scp::Class)) {
            assert(class_node->cls());
            class_node->cls()->add_type_def(node);
        } else if (scope()->is(scp::ClassTpl)) {
            assert(class_node->cls_tpl());
            class_node->cls_tpl()->add_type_def(node);
        } else {
            assert(false);
        }
        sync_scope(node);
    } else {
        // transient typedef (in function body)
        Ptr<UserType> type = make<AliasType>(nullptr, node);
        scope()->set(alias_id, std::move(type));
    }
}

void Init::visit(Ref<ast::VarDefList> node) {
    // visit `TypeName`
    assert(node->has_type_name());
    node->type_name()->accept(*this);

    if (!scope()->is(scp::Persistent))
        return;

    // add module/class/tpl variables/properties
    auto class_node = class_def();
    for (unsigned n = 0; n < node->def_num(); ++n) {
        auto def = node->def(n);
        auto name_id = def->name().str_id();

        // visit value expr for possible `TypeName`s
        if (def->has_default_value())
            def->default_value()->accept(*this);

        // already in current scope?
        if (scope()->has(name_id, true)) {
            diag().error(def, "already defined");
            continue;
        }

        auto install = [&](auto&& var) {
            if (class_node) {
                // set class const/prop
                auto var_ref = ref(var);
                if (class_node->cls()) {
                    auto cls = class_node->cls();
                    var->set_cls(cls);
                    cls->set(name_id, std::move(var));
                } else {
                    assert(class_node->cls_tpl());
                    auto tpl = class_node->cls_tpl();
                    tpl->set(name_id, std::move(var));
                }
                scope()->set(name_id, var_ref);
            } else {
                // add to module scope
                scope()->set(name_id, std::move(var));
            }
        };

        // flags
        Var::Flag flags = Var::NoFlags;
        if (node->is_const())
            flags |= Var::Const;
        if (class_node->cls_tpl())
            flags |= Var::Tpl;

        // create and add
        if (class_node && !node->is_const()) {
            auto prop = make<Prop>(node->type_name(), def, Ref<Type>{}, flags);
            auto prop_ref = ref(prop);
            install(std::move(prop));
            def->set_prop(prop_ref);
        } else {
            auto var = make<Var>(node->type_name(), def, Ref<Type>{}, flags);
            auto var_ref = ref(var);
            install(std::move(var));
            def->set_var(var_ref);
        }
        def->set_scope_version(scope()->version());
    }
}

bool Init::do_visit(Ref<ast::FunDef> node) {
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

    auto name_id = node->name_id();
    auto sym = cls_base->get(name_id);
    if (sym && !sym->is<FunSet>()) {
        diag().error(
            node->loc_id(), str(name_id).size(),
            "defined and is not a function");
        return false;
    }

    cls_base->add_fun(node);
    sync_scope(node);
    return true;
}

// TODO: do not set type/tpl, do it in resolver
bool Init::do_visit(Ref<ast::TypeName> node) {
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

void Init::export_classes() {
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
                diag().error(
                    cls_base->node()->loc_id(), str(name_id).size(),
                    "defined in multiple modules");
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
        diag().error(
            item.node->loc_id(), str(name_id).size(), "failed to resolve");
    }
}

} // namespace ulam::sema
