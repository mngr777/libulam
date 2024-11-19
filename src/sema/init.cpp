#include "libulam/sema/visitor.hpp"
#include <cassert>
#include <libulam/diag.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/init.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/ph_type.hpp>

namespace ulam::sema {

bool Init::visit(ast::Ref<ast::Root> node) {
    assert(!node->program());
    // make program
    node->set_program(ulam::make<Program>(node));
    return RecVisitor::visit(node);
}

bool Init::visit(ast::Ref<ast::ModuleDef> node) {
    assert(!node->module());
    // make module
    auto module = ulam::make<Module>(node);
    node->set_module(ulam::ref(module));
    init_classes(ulam::ref(module), node);
    program()->add_module(std::move(module));
    return RecVisitor::visit(node);
}

bool Init::visit(ast::Ref<ast::ClassDef> node) {
    return RecVisitor::visit(node);
}

bool Init::do_visit(ast::Ref<ast::TypeDef> node) {
    // set placeholder for alias type
    auto alias_str_id = node->name().str_id();
    if (scope()->has(alias_str_id, true)) {
        // TODO: after types are resolved, report error if types don't match
        return false;
    }
    scope()->set_placeholder(alias_str_id);
    return true;
}

bool Init::do_visit(ast::Ref<ast::TypeSpec> node) {
    if (!node->is_builtin())
        return true;
    // add specs of unseen types to module imports
    assert(module_def()->module());
    auto str_id = node->ident()->name().str_id();
    if (!scope()->has(str_id, Scope::Module))
        module_def()->module()->add_import(node);
    return false;
}

void Init::init_classes(Ref<Module> module, ast::Ref<ast::ModuleDef> node) {
    for (unsigned n = 0; n < node->child_num(); ++n) {
        auto& child_v = node->get(n);
        if (ast::is<ast::ClassDef>(child_v))
            init_class(module, ast::as_ref<ast::ClassDef>(child_v));
    }
}

void Init::init_class(Ref<Module> module, ast::Ref<ast::ClassDef> node) {
    assert(!node->type());
    auto name_id = node->name().str_id();
    // already defined?
    auto prev = module->get_class(name_id);
    if (prev) {
        diag().emit(
            diag::Error, node->name().loc_id(), str(name_id).size(),
            "already defined"); // TODO: say where
        return;
    }
    // add to module, set node attr
    auto cls = ulam::make<Class>(program()->next_type_id(), node);
    auto cls_ref = ref(cls);
    module->add_class(std::move(cls));
    node->set_type(cls_ref);
}

} // namespace ulam::sema
