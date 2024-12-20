#include <cassert>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/class_tpl.hpp>

#define ULAM_DEBUG
#define ULAM_DEBUG_PREFIX "[Class] "
#include "src/debug.hpp"

namespace ulam {

Class::Class(TypeIdGen* id_gen, std::string_view name, Ref<ClassTpl> tpl):
    UserType{id_gen},
    ClassBase{tpl->node(), tpl->param_scope()->parent(), scp::Class},
    _name{name},
    _tpl{tpl} {
    assert(id_gen);
}

Class::Class(
    TypeIdGen* id_gen,
    std::string_view name,
    Ref<ast::ClassDef> node,
    Ref<Scope> scope):
    UserType{id_gen}, ClassBase{node, scope, scp::Class}, _name{name}, _tpl{} {
    assert(id_gen);
}

Class::~Class() {}

str_id_t Class::name_id() const { return node()->name().str_id(); }

bitsize_t Class::bitsize() const { assert(false); /* not implemented */ }

void Class::add_param_var(Ptr<Var>&& var) {
    assert(var->is(Var::ClassParam | Var::Const));
    auto name_id = var->name_id();
    assert(!param_scope()->has(name_id));
    assert(!has(name_id));
    _param_vars.push_back(ref(var));
    param_scope()->set(name_id, ref(var));
    set(name_id, std::move(var));
}

Ref<cls::Layout> Class::layout() {
    if (_layout)
        init_layout();
    return ref(_layout);
}

void Class::init_layout() {
    auto layout = make<cls::Layout>();
    for (auto& pair : members()) {
        auto& [name_id, sym] = pair;
        if (!sym.owns() || !sym.is<Var>())
            continue;
        auto var = sym.get<Var>();
        layout->add(var);
    }
    // for (auto& anc : _ancestors) {
    //     // layout->add()
    //     // TODO
    // }
    _layout.swap(layout);
}

} // namespace ulam
