#include <iomanip>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/sema/debug/out.hpp>
#include <libulam/sema/visitor.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope/iter.hpp>

namespace ulam::sema::dbg {

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

void Out::print_class_registry() {
    const auto& list = _program->classes().list();
    hr();
    _os << "# class registry (" << list.size() << "):\n";
    hr();
    for (const auto cls : list)
        _os << std::setw(5) << cls->class_id() << " " << cls->name() << "\n";
    hr2();
}

const std::string_view Out::line_at(loc_id_t loc_id) {
    return _program->src_man().line_at(loc_id);
}

const std::string_view Out::line_at(Ref<ast::Node> node) {
    return line_at(node->loc_id());
}

const std::string_view Out::str(str_id_t str_id) const {
    return str_pool().get(str_id);
}

const std::string_view Out::text(str_id_t str_id) const {
    return text_pool().get(str_id);
}

const UniqStrPool& Out::str_pool() const { return _program->str_pool(); }
const UniqStrPool& Out::text_pool() const { return _program->text_pool(); }

void Out::hr() { _os << "--------------------\n"; }

void Out::hr2() { _os << "====================\n"; }

} // namespace ulam::sema::dbg
