#include "libulam/str_pool.hpp"
#include <cassert>
#include <libulam/diag.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/resolve_deps.hpp>
#include <libulam/sema/visitor.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/var.hpp>
#include <set>
#include <unordered_map>

namespace ulam::sema {

bool ResolveDeps::visit(ast::Ref<ast::Root> node) {
    assert(!node->program());
    // make program
    node->set_program(ulam::make<Program>(diag(), node));
    RecVisitor::visit(node);
    check_module_deps();
    return false;
}

bool ResolveDeps::visit(ast::Ref<ast::ModuleDef> node) {
    assert(!node->module());
    // make module
    auto module = ulam::make<Module>(node);
    node->set_module(ulam::ref(module));
    init_classes(ulam::ref(module), node);
    program()->add_module(std::move(module));
    return RecVisitor::visit(node);
}

bool ResolveDeps::visit(ast::Ref<ast::VarDefList> node) {
    // don't skip the type name
    node->type_name()->accept(*this);
    if (!scope()->is(Scope::Module) && !scope()->is(Scope::Class))
        return false;
    // create and set module/class/tpl variables
    assert(!scope()->is(Scope::Module) || node->is_const());
    auto class_node = class_def();
    for (unsigned n = 0; n < node->def_num(); ++n) {
        auto def_node = node->def(n);
        auto name_id = def_node->name().str_id();
        // name is already in current scope?
        if (scope()->has(name_id, true)) {
            // TODO: where?
            diag().emit(
                diag::Error, def_node->loc_id(), 1, "variable already defined");
            continue;
        }
        Var::Flag flags = node->is_const() ? Var::IsConst : Var::NoFlags;
        auto var =
            ulam::make<Var>(node->type_name(), def_node, Ref<Type>{}, flags);
        auto var_ref = ref(var);
        if (class_node) {
            // class/tpl variable
            if (class_node->type()) {
                class_node->type()->set(name_id, std::move(var));
            } else {
                assert(class_node->type_tpl());
                class_node->type_tpl()->set(name_id, std::move(var));
            }
        } else {
            // module constant, move to node
            def_node->set_var(std::move(var));
        }
        // add to scope
        scope()->set(name_id, var_ref);
    }
    return false;
}

bool ResolveDeps::do_visit(ast::Ref<ast::TypeDef> node) {
    auto alias_id = node->name().str_id();
    // name is already in current scope?
    if (scope()->has(alias_id, true)) {
        // NOTE: after types are resolves, complain if types don't match
        return false;
    }
    // make alias type
    auto type =
        ulam::make<AliasType>(program()->next_type_id(), node, Ref<Type>{});
    auto type_ref = ref(type);
    // set node attr
    node->set_alias_type(std::move(type));
    // add to scope
    scope()->set(alias_id, type_ref);
    // if in class/tpl scope, add to class/tpl
    if (scope()->is(Scope::Class)) {
        auto class_node = class_def();
        assert(class_node);
        if (class_node->type()) {
            class_node->type()->set(alias_id, type_ref);
        } else {
            assert(class_node->type_tpl());
            class_node->type_tpl()->set(alias_id, type_ref);
        }
    }
    return true;
}

bool ResolveDeps::do_visit(ast::Ref<ast::FunDef> node) {
    auto class_node = class_def();
    assert(class_node);
    auto name_id = node->name().str_id();
    Ref<Fun> fun{};
    if (class_node->type()) {
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
    fun->add_overload(node);
    return true;
}

bool ResolveDeps::do_visit(ast::Ref<ast::TypeName> node) {
    assert(node->first());
    auto spec = node->first();
    if (spec->is_builtin())
        return false;
    // add specs of unseen types to module imports
    assert(module_def()->module());
    auto str_id = spec->ident()->name().str_id();
    if (!scope()->has(str_id, Scope::Module))
        module_def()->module()->add_import(spec);
    return false;
}

void ResolveDeps::init_classes(
    Ref<Module> module, ast::Ref<ast::ModuleDef> node) {
    for (unsigned n = 0; n < node->child_num(); ++n) {
        auto& child_v = node->get(n);
        if (ast::is<ast::ClassDef>(child_v))
            init_class(module, ast::as_ref<ast::ClassDef>(child_v));
    }
}

void ResolveDeps::init_class(Ref<Module> module, ast::Ref<ast::ClassDef> node) {
    assert(!node->type());
    auto name_id = node->name().str_id();
    // already defined?
    auto prev = module->get(name_id);
    if (prev) {
        diag().emit(
            diag::Error, node->name().loc_id(), str(name_id).size(),
            "already defined"); // TODO: say where
        return;
    }
    // add to module, set node attr
    if (node->params()) {
        // class tpl
        auto params = node->params();
        assert(params->child_num() > 0);
        // make class tpl
        auto tpl = ulam::make<ClassTpl>(program(), node);
        auto tpl_ref = ref(tpl);
        // add params
        for (unsigned n = 0; n < params->child_num(); ++n) {
            auto param = params->get(n);
            auto var = ulam::make<Var>(
                param->type_name(), param, Ref<Type>{}, Var::ClassParam);
            tpl->set(param->name().str_id(), std::move(var));
        }
        module->set<TypeTpl>(name_id, std::move(tpl));
        node->set_type_tpl(tpl_ref);
    } else {
        // class
        auto cls = ulam::make<Class>(program()->next_type_id(), node);
        auto cls_ref = ref(cls);
        module->set<Type>(name_id, std::move(cls));
        node->set_type(cls_ref);
    }
}

void ResolveDeps::check_module_deps() {
    using ModuleSet = std::set<Ref<Module>>;
    std::unordered_map<str_id_t, ModuleSet> exporting_modules;
    // collect all exports (with possible duplicates)
    for (auto& module : program()->modules()) {
        for (auto& pair : *module) {
            auto name_id = pair.first;
            auto it = exporting_modules.find(name_id);
            if (it == exporting_modules.end()) {
                exporting_modules.emplace(name_id, ModuleSet{ref(module)});
            } else {
                it->second.insert(ref(module));
            }
        }
    }
    // check for duplicates
    bool has_dups = false;
    for (auto& pair : exporting_modules) {
        auto name_id = pair.first;
        if (pair.second.size() > 1) {
            has_dups = true;
            // TODO: locations, module names
            diag().emit(
                diag::Error,
                std::string(str(name_id)) + " defined in multiple modules");
        }
    }
    if (has_dups)
        diag().emit(diag::Fatal, "duplicate class definitions");
    // check for unresolved dependencies
    for (auto& module : program()->modules()) {
        for (auto& pair : module->imports()) {
            auto name_id = pair.first;
            if (exporting_modules.find(name_id) == exporting_modules.end()) {
                // TODO: locations, module name
                diag().emit(
                    diag::Error, std::string("unresolved dependency ") +
                                     std::string(str(name_id)));
            }
        }
    }
}

} // namespace ulam::sema
