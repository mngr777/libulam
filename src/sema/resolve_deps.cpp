#include <cassert>
#include <libulam/diag.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/resolve_deps.hpp>
#include <libulam/sema/visitor.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/class_kind.hpp>
#include <libulam/semantic/var.hpp>
#include <libulam/str_pool.hpp>

#define ULAM_DEBUG
#define ULAM_DEBUG_PREFIX "[sema::ResolveDeps] "
#include "src/debug.hpp"

namespace ulam::sema {

bool ResolveDeps::visit(ast::Ref<ast::Root> node) {
    assert(!node->program());
    // make program
    node->set_program(ulam::make<Program>(diag(), node));
    RecVisitor::visit(node);
    return false;
}

bool ResolveDeps::visit(ast::Ref<ast::ModuleDef> node) {
    assert(!node->module());
    // make module
    auto module = program()->add_module(node);
    node->set_module(module);
    init_classes(module);
    return RecVisitor::visit(node);
}

bool ResolveDeps::visit(ast::Ref<ast::VarDefList> node) {
    // don't skip the type name
    node->type_name()->accept(*this);
    // create and set module/class/tpl variables
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
            // module constant
            scope()->set(name_id, std::move(var));
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
        // TODO: after types are resolves, complain if types don't match
        return false;
    }
    auto class_node = class_def();
    if (class_node) {
        // class typedef
        Ref<Type> type_ref{};
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
        scope()->set(alias_id, std::move(type));
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
    // TODO
    return false;
}

void ResolveDeps::init_classes(Ref<Module> module) {
    auto node = module->node();
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
        assert(node->params()->child_num() > 0);
        assert(node->kind() != ClassKind::Element);
        // class tpl
        auto params = node->params();
        // make class tpl
        auto tpl = ulam::make<ClassTpl>(module, node);
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
        auto cls = ulam::make<Class>(module, node);
        auto cls_ref = ref(cls);
        module->set<Type>(name_id, std::move(cls));
        node->set_type(cls_ref);
    }
}

} // namespace ulam::sema
