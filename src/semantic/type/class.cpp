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

bitsize_t Class::bitsize() const {
    bitsize_t size = direct_bitsize();
    for (const auto& anc : _ancestry.ancestors())
        size += anc->cls()->direct_bitsize();
    return size;
}

bitsize_t Class::direct_bitsize() const {
    bitsize_t size = 0;
    for (auto& [_, sym] : members()) {
        if (!sym.is<Prop>())
            continue;
        auto var = sym.get<Prop>();
        if (!var->is_ready())
            continue;
        size += var->type()->bitsize();
    }
    return size;
}

RValue Class::construct() {
    return make_s<Object>(this);
}

void Class::add_param_var(Ptr<Var>&& var) {
    assert(var->is(Var::ClassParam | Var::Const));
    auto name_id = var->name_id();
    assert(!param_scope()->has(name_id));
    assert(!has(name_id));
    _param_vars.push_back(ref(var));
    param_scope()->set(name_id, ref(var));
    set(name_id, std::move(var));
}

void Class::add_ancestor(Ref<Class> cls, Ref<ast::TypeName> node) {
    // TODO: merge fun sets?
    if (_ancestry.add(cls, node))
        cls->members().export_symbols(members());
}

} // namespace ulam
