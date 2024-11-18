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
    program()->add_module(std::move(module));
    return RecVisitor::visit(node);
}

bool Init::visit(ast::Ref<ast::ClassDef> node) {
    assert(!node->type());
    // make class type
    auto type = ulam::make<Class>(program()->next_type_id(), node);
    auto type_ref = ulam::ref(type);
    // add to scope
    auto name_id = node->name().str_id();
    if (scope()->has(name_id, Scope::Module)) {
        // TODO: already defined where?
        diag().emit(
            diag::Error, node->name().loc_id(), str(name_id).size(),
            "already defined");
        return {};
    }
    assert(scope()->is(Scope::Module));
    scope()->set(name_id, std::move(type));
    // set node attr
    node->set_type(type_ref);
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

} // namespace ulam::sema
