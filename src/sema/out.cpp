#include "src/sema/out.hpp"
#include <libulam/ast/nodes/module.hpp>
#include <libulam/sema/visitor.hpp>

namespace ulam::sema {

// void Out::print(Ref<Scope> scope) {
//     assert(false);
//     if (scope->flags() == scp::NoFlags) {
//         _os << "<no flags>";
//     } else {
//         if (scope->flags() & scp::Program)
//             _os << "[program]";
//         if (scope->flags() & scp::ModuleEnv)
//             _os << "[module-env]";
//         if (scope->flags() & scp::Module)
//             _os << "[module]";
//         if (scope->flags() & scp::ClassTpl)
//             _os << "[class-tpl]";
//         if (scope->flags() & scp::Class)
//             _os << "[class]";
//     }
//     _os << "\n";
//     scope->for_each([&](str_id_t name_id, Scope::Symbol& sym) {
//         _os << str(name_id) << ": ";
//         print(&sym);
//         _os << "\n";
//     });
//     if (scope->parent()) {
//         _os << "----------\n";
//         print(scope->parent());
//     } else {
//         _os << "==========\n";
//     }
// }

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
    if (sym->is<UserType>()) {
        _os << "type ";
        auto type = sym->get<UserType>();
        if (type->is_alias()) {
            _os << "(alias)";
        } else if (type->is_class()) {
            _os << "(class)";
        }
    } else if (sym->is<ClassTpl>()) {
        _os << "tpl";
    } else if (sym->is<Var>()) {
        _os << "var";
    } else if (sym->is<FunSet>()) {
        _os << "fun";
    }
}

void Out::print(Ref<Type> type, bool canon) {
    // std::list<array_size_t> array_sizes;
}

void Out::print(Ref<Var> var) {
    if (var->type()) {
        print(var->type());
    } else {
        _os << "<no type>";
    }
    _os << " " << str(var->name_id());
    _os << "\n";
}

std::string_view Out::str(str_id_t str_id) {
    return _program->str_pool().get(str_id);
}

} // namespace ulam::sema
