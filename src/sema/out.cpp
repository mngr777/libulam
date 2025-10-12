#include "src/sema/out.hpp"
#include <iomanip>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/sema/visitor.hpp>
#include <libulam/semantic/scope/iter.hpp>

namespace ulam::sema {

void Out::print(Scope& scope) {
    hr2();
    for (auto cur = &scope; cur; cur = cur->parent()) {
        if (cur->flags() == scp::NoFlags) {
            _os << "<no flags>\n";
        } else {
            if (cur->flags() & scp::Program)
                _os << "[program]\n";
            if (cur->flags() & scp::ModuleEnv)
                _os << "[module-env]\n";
            if (cur->flags() & scp::Module)
                _os << "[module]\n";
            if (cur->flags() & scp::ClassTpl)
                _os << "[class-tpl]\n";
            if (cur->flags() & scp::Class)
                _os << "[class]\n";
        }

        for (auto [name_id, sym] : *cur) {
            _os << std::setw(16) << " " << str(name_id) << " ";
            print(*sym);
            _os << "\n";
        }
        if (cur->parent())
            hr();
    }
    hr2();
}

void Out::print(Scope::Symbol& sym) {
    sym.accept(
        [&](Ref<UserType> type) { _os << "type"; },
        [&](Ref<ClassTpl> tpl) { _os << "tpl"; },
        [&](Ref<FunSet> fset) { _os << "fun"; },
        [&](Ref<Var> var) { _os << "var"; },
        [&](Ref<Prop> prop) { _os << "prop"; });
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

void Out::hr() { _os << "--------------------\n"; }

void Out::hr2() { _os << "====================\n"; }

} // namespace ulam::sema
