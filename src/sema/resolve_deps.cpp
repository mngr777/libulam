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

bool ResolveDeps::visit(ast::Ref<ast::Root> node) {
    assert(!node->program());
    // make program
    node->set_program(ulam::make<Program>(diag(), node));
    RecVisitor::visit(node);
    export_classes();
    return false;
}

bool ResolveDeps::visit(ast::Ref<ast::ModuleDef> node) {
    assert(!node->module());
    auto module = program()->add_module(node);
    node->set_module(module);
    return RecVisitor::visit(node);
}

bool ResolveDeps::do_visit(ast::Ref<ast::ClassDef> node) {
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
    // add to module, set node attr
    if (node->params()) {
        assert(node->params()->child_num() > 0);
        assert(node->kind() != ClassKind::Element);
        // class tpl
        auto params = node->params();
        // make class tpl
        auto tpl = ulam::make<ClassTpl>(module(), node);
        auto tpl_ref = ref(tpl);
        // add params
        for (unsigned n = 0; n < params->child_num(); ++n) {
            auto param = params->get(n);
            auto var = ulam::make<Var>(
                param->type_name(), param, Ref<Type>{}, Var::ClassParam);
            tpl->set(param->name().str_id(), std::move(var));
        }
        module()->set<TypeTpl>(name_id, std::move(tpl));
        node->set_type_tpl(tpl_ref);
    } else {
        // class
        auto cls = ulam::make<Class>(module(), node);
        auto cls_ref = ref(cls);
        module()->set<Type>(name_id, std::move(cls));
        node->set_type(cls_ref);
    }
    return true;
}

bool ResolveDeps::visit(ast::Ref<ast::TypeDef> node) {
    // don't skip the type name
    visit(node->type_name());
    // create alias type
    auto alias_id = node->name().str_id();
    // name is already in current scope?
    if (scope()->has(alias_id, true)) {
        // TODO: after types are resolves, complain if types don't match
        return false;
    }
    auto class_node = class_def();
    Ref<Type> type_ref{};
    if (class_node) {
        // class typedef
        if (class_node->type()) {
            // set class member
            auto cls = class_node->type();
            Ptr<Type> type{
                ulam::make<AliasType>(program()->next_type_id(), node, cls)};
            type_ref = ref(type);
            cls->set(alias_id, std::move(type));
        } else {
            // set tpl member
            assert(class_node->type_tpl());
            auto tpl = class_node->type_tpl();
            Ptr<Type> type{ulam::make<AliasType>(NoTypeId, node)};
            type_ref = ref(type);
            tpl->set(alias_id, std::move(type));
        }
        scope()->set(alias_id, type_ref);
    } else {
        // module typedef (is not a module member)
        Ptr<Type> type{ulam::make<AliasType>(NoTypeId, node)};
        type_ref = ref(type);
        scope()->set(alias_id, std::move(type));
    }
    // set node attr
    node->set_alias_type(type_ref->basic()->as_alias());
    return {};
}

bool ResolveDeps::visit(ast::Ref<ast::VarDefList> node) {
    // don't skip the type name
    visit(node->type_name());
    // create and set module/class/tpl variables
    auto class_node = class_def();
    for (unsigned n = 0; n < node->def_num(); ++n) {
        auto def = node->def(n);
        auto name_id = def->name().str_id();
        // name is already in current scope?
        if (scope()->has(name_id, true)) {
            // TODO: where?
            diag().emit(
                diag::Error, def->loc_id(), 1, "variable already defined");
            continue;
        }
        Var::Flag flags = node->is_const() ? Var::IsConst : Var::NoFlags;
        auto var = ulam::make<Var>(node->type_name(), def, Ref<Type>{}, flags);
        auto var_ref = ref(var);
        if (class_node) {
            // class/tpl variable
            if (class_node->type()) {
                class_node->type()->set(name_id, std::move(var));
            } else {
                assert(class_node->type_tpl());
                class_node->type_tpl()->set(name_id, std::move(var));
            }
            // add to scope
            scope()->set(name_id, var_ref);
        } else {
            // module constant
            scope()->set(name_id, std::move(var));
        }
        // set node attr
        def->set_var(var_ref);
    }
    return {};
}

bool ResolveDeps::visit(ast::Ref<ast::FunDef> node) {
    // don't skip ret type...
    visit(node->ret_type_name());
    // and params
    visit(node->params());
    // create and set fun
    auto class_node = class_def();
    assert(class_node);
    auto name_id = node->name().str_id();
    Ref<Fun> fun{};
    if (class_node->type()) {
        // class fun
        auto cls = class_node->type();
        auto sym = cls->get(name_id);
        if (sym) {
            if (!sym->is<Fun>()) {
                diag().emit(
                    diag::Error, node->name().loc_id(), str(name_id).size(),
                    "defined and is not a function");
                return false;
            }
        } else {
            sym = cls->set(name_id, ulam::make<Fun>());
        }
        fun = sym->get<Fun>();
    } else {
        // class tpl fun
        assert(class_node->type_tpl());
        auto cls_tpl = class_node->type_tpl();
        auto sym = cls_tpl->get(name_id);
        if (sym) {
            if (!sym->is<Fun>()) {
                diag().emit(
                    diag::Error, node->name().loc_id(), str(name_id).size(),
                    "defined and is not a function");
            }
        } else {
            sym = cls_tpl->set(name_id, ulam::make<Fun>());
        }
        fun = sym->get<Fun>();
    }
    // create overload
    auto overload = fun->add_overload(node);
    // set node attr
    node->set_overload(overload);
    // add to scope
    if (!scope()->has(name_id)) {
        scope()->set(name_id, fun);
    }
    return {};
}

bool ResolveDeps::do_visit(ast::Ref<ast::TypeName> node) {
    // set type/tpl TypeSpec attr
    // NOTE: any name not in scope has to be imported and
    // imported names can be later unambigously resolved without scope
    auto type_spec = node->first();
    if (type_spec->is_builtin()) {
        // builtin type/tpl
        BuiltinTypeId id = type_spec->builtin_type_id();
        if (has_bitsize(id)) {
            type_spec->set_type_tpl(program()->builtin_type_tpl(id));
        } else if (is_prim(id)) {
            type_spec->set_type(program()->builtin_type(id));
        }
        return false;
    }
    auto name_id = type_spec->ident()->name().str_id();
    if (scope()->has(name_id)) {
        // set type/tpl
        auto sym = scope()->get(name_id);
        if (sym->is<Type>()) {
            type_spec->set_type(sym->get<Type>());
        } else {
            assert(sym->is<TypeTpl>());
            type_spec->set_type_tpl(sym->get<TypeTpl>());
        }
    } else {
        // add external dependency
        module()->add_dep(name_id);
        // postpone until all exports are known (module ID to avoid importing
        // from itself)
        _unresolved.push_back({module()->id(), node});
    }
    return false;
}

void ResolveDeps::enter_module_scope(Ref<Module> module) {
    enter_scope(module->scope());
}

void ResolveDeps::enter_class_scope(Ref<Class> cls) {
    enter_scope(cls->scope());
}

void ResolveDeps::enter_tpl_scope(Ref<ClassTpl> tpl) {
    enter_scope(tpl->scope());
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
            assert(sym.is<Type>());
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
            if (it != exporting.end()) {
                assert(it->second.size() == 1);
                auto& exporter = *it->second.begin();
                auto sym = exporter->get(name_id);
                assert(sym);
                if (sym->is<Type>()) {
                    mod->add_import(name_id, exporter, sym->get<Type>());
                } else {
                    assert(sym->is<TypeTpl>());
                    mod->add_import(name_id, exporter, sym->get<TypeTpl>());
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
                if (sym->is<Type>()) {
                    type_spec->set_type(sym->get<Type>());
                } else {
                    assert(sym->is<TypeTpl>());
                    type_spec->set_type_tpl(sym->get<TypeTpl>());
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
