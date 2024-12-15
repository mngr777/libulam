#include <cassert>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/class_tpl.hpp>

#define ULAM_DEBUG
#define ULAM_DEBUG_PREFIX "[Class] "
#include "src/debug.hpp"

namespace ulam {

Class::Class(TypeIdGen* id_gen, Ref<ClassTpl> tpl):
    UserType{id_gen},
    ClassBase{tpl->node(), tpl->param_scope()->parent(), scp::Class},
    _tpl{tpl} {
    assert(id_gen);
}

Class::Class(TypeIdGen* id_gen, Ref<ast::ClassDef> node, Ref<Scope> scope):
    UserType{id_gen}, ClassBase{node, scope, scp::Class}, _tpl{} {
    assert(id_gen);
}

Class::~Class() {}

str_id_t Class::name_id() const { return node()->name().str_id(); }

bitsize_t Class::bitsize() const { assert(false); /* not implemented */ }

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
