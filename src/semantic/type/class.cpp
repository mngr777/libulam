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

bool Class::is_base_of(Ref<const Class> other) const {
    return other->_ancestry.is_base(this);
}

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

RValue Class::construct() { return RValue{make_s<Object>(this)}; }

RValue Class::load(const BitVectorView data, BitVector::size_t off) const {
    // TMP: const cast hack
    return RValue{make_s<Object>(
        const_cast<Class*>(this), data.view(off, bitsize()).copy())};
}

void Class::store(
    BitVectorView data, BitVector::size_t off, const RValue& rval) const {}

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

bool Class::is_castable_to(Ref<const Type> type, bool expl) const {
    return conversion(type, expl).size() == 1;
}

bool Class::is_castable_to(BuiltinTypeId builtin_type_id, bool expl) const {
    return conversion(builtin_type_id, expl).size() == 1;
}

// TODO: conversion functions to be reviewed

Class::ConversionMatchRes
Class::conversion(Ref<const Type> type, bool expl) const {
    auto canon = type->canon();
    {
        auto it = _conversions.find(type->canon()->id());
        if (it != _conversions.end())
            return {it->second};
    }
    ConversionMatchRes matches{};
    ConversionMatchRes bi_matches{};
    for (auto [_, fun] : _conversions) {
        auto ret_canon = fun->ret_type()->canon();
        if (canon->is_builtin() &&
            canon->builtin_type_id() == ret_canon->builtin_type_id()) {
            bi_matches.insert(fun);
        } else if (
            bi_matches.size() == 0 && ret_canon->is_castable_to(type, expl)) {
            matches.insert(fun);
        }
    }
    return (bi_matches.size() > 0) ? bi_matches : matches;
}

Class::ConversionMatchRes
Class::conversion(BuiltinTypeId builtin_type_id, bool expl) const {
    ConversionMatchRes matches{};
    ConversionMatchRes bi_matches{};
    for (auto [_, fun] : _conversions) {
        auto ret_canon = fun->ret_type()->canon();
        if (ret_canon->is_builtin() &&
            ret_canon->builtin_type_id() == builtin_type_id) {
            bi_matches.insert(fun);
        } else if (
            bi_matches.size() == 0 &&
            ret_canon->is_castable_to(builtin_type_id, expl)) {
            matches.insert(fun);
        }
    }
    return (bi_matches.size() > 0) ? bi_matches : matches;
}

void Class::add_conversion(Ref<Type> type, Ref<Fun> fun) {
    auto canon = type->canon();
    assert(fun->cls() == this);
    assert(_conversions.count(canon->id()) == 0);
    _conversions[canon->id()] = fun;
}

} // namespace ulam
