#include "src/sema/out.hpp"
#include "libulam/sema/visitor.hpp"
#include <libulam/ast/nodes/module.hpp>

namespace ulam::sema {

void Out::print(Ref<Scope> scope) {
    assert(false);
    if (scope->flags() == Scope::NoFlags) {
        _os << "<no flags>";
    } else {
        if (scope->flags() & Scope::Program)
            _os << "[program]";
        if (scope->flags() & Scope::ModuleEnv)
            _os << "[module-env]";
        if (scope->flags() & Scope::Module)
            _os << "[module]";
        if (scope->flags() & Scope::ClassTpl)
            _os << "[class-tpl]";
        if (scope->flags() & Scope::Class)
            _os << "[class]";
    }
    _os << "\n";
    scope->for_each([&](str_id_t name_id, Scope::Symbol& sym) {
        _os << str(name_id) << ": ";
        print(&sym);
        _os << "\n";
    });
    if (scope->parent()) {
        _os << "----------\n";
        print(scope->parent());
    } else {
        _os << "==========\n";
    }
}

void Out::print(RecVisitor::Pass pass) {
    switch (pass) {
    case RecVisitor::Pass::Module:
        _os << "pass: module\n";
        return;
    case RecVisitor::Pass::Classes:
        _os << "pass: classes\n";
        return;
    case RecVisitor::Pass::FunBodies:
        _os << "pass: fun bodies\n";
    }
}

void Out::print(Scope::Symbol* sym) {
    if (sym->is<Type>()) {
        _os << "type ";
        auto type = sym->get<Type>();
        if (type->basic()->is_alias()) {
            _os << "(alias)";
        } else if (type->basic()->is_class()) {
            _os << "(class)";
        }
    } else if (sym->is<TypeTpl>()) {
        _os << "tpl";
    } else if (sym->is<Var>()) {
        _os << "var";
    } else if (sym->is<Fun>()) {
        _os << "fun";
    }
}

std::string_view Out::str(str_id_t str_id) {
    return _program->ast()->ctx().str(str_id);
}

} // namespace ulam::sema
