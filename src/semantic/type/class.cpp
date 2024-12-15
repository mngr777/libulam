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

} // namespace ulam
