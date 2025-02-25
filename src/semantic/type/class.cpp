#include <algorithm>
#include <cassert>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/ast/nodes/type.hpp>
#include <libulam/ast/nodes/var_decl.hpp>
#include <libulam/semantic/module.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/class/prop.hpp>
#include <libulam/semantic/type/class_tpl.hpp>
#include <libulam/semantic/type/prim.hpp>
#include <libulam/semantic/value.hpp>

#define ULAM_DEBUG
#define ULAM_DEBUG_PREFIX "[Class] "
#include "src/debug.hpp"

namespace ulam {

Class::Class(std::string_view name, Ref<ClassTpl> tpl):
    UserType{&tpl->module()->program()->type_id_gen()},
    ClassBase{tpl->node(), tpl->module(), scp::Class},
    _name{name},
    _tpl{tpl} {}

Class::Class(
    std::string_view name, Ref<ast::ClassDef> node, Ref<Module> module):
    UserType{&module->program()->type_id_gen()},
    ClassBase{node, module, scp::Class},
    _name{name},
    _tpl{} {}

Class::~Class() {}

str_id_t Class::name_id() const { return node()->name().str_id(); }

Ref<Var>
Class::add_param(Ref<ast::TypeName> type_node, Ref<ast::VarDecl> node) {
    auto var = ClassBase::add_param(type_node, node);
    var->set_cls(this);
    return var;
}

Ref<Var> Class::add_param(
    Ref<ast::TypeName> type_node, Ref<ast::VarDecl> node, Value&& val) {
    auto var = add_param(type_node, node);
    var->set_value(std::move(val));
    return var;
}

Ref<AliasType> Class::add_type_def(Ref<ast::TypeDef> node) {
    auto type = ClassBase::add_type_def(node);
    type->set_cls(this);
    return type;
}

Ref<Fun> Class::add_fun(Ref<ast::FunDef> node) {
    auto fun = ClassBase::add_fun(node);
    fun->set_cls(this);
    return fun;
}

Ref<Var>
Class::add_const(Ref<ast::TypeName> type_node, Ref<ast::VarDecl> node) {
    auto var = ClassBase::add_const(type_node, node);
    var->set_cls(this);
    return var;
}

Ref<Prop>
Class::add_prop(Ref<ast::TypeName> type_node, Ref<ast::VarDecl> node) {
    auto prop = ClassBase::add_prop(type_node, node);
    prop->set_cls(this);
    return prop;
}

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
    bitsize_t total = 0;
    for (auto prop : props()) {
        if (!prop->is_ready())
            continue;
        auto size = prop->type()->bitsize();
        if (kind() == ClassKind::Union) {
            total = std::max(total, size);
        } else {
            total += size;
        }
    }
    return total;
}

RValue Class::construct() const {
    return RValue{make_s<Object>(const_cast<Class*>(this))};
}

RValue Class::load(const BitVectorView data, BitVector::size_t off) const {
    // TMP: const cast hack
    return RValue{make_s<Object>(
        const_cast<Class*>(this), data.view(off, bitsize()).copy())};
}

void Class::store(
    BitVectorView data, BitVector::size_t off, const RValue& rval) const {}

void Class::add_ancestor(Ref<Class> cls, Ref<ast::TypeName> node) {
    // TODO: merge fun sets?
    if (_ancestry.add(cls, node))
        cls->members().export_symbols(members());
}

// NOTE: for ambiguous conversion truth is returned,
// the error is to be catched when applying conversions

bool Class::is_castable_to(Ref<const Type> type, bool expl) const {
    return !convs(type, expl).empty();
}

bool Class::is_castable_to(BuiltinTypeId bi_type_id, bool expl) const {
    return !convs(bi_type_id, expl).empty();
}

conv_cost_t Class::conv_cost(Ref<const Type> type, bool allow_cast) const {
    return is_same(type) ? 0 : convs(type, allow_cast).cost();
}

// TODO: handle non-primitive builtins

ConvList Class::convs(Ref<const Type> type, bool allow_cast) const {
    auto canon_ = type->canon();
    ConvList res;
    {
        // has exact conversion?
        auto it = _convs.find(canon_->id());
        if (it != _convs.end()) {
            res.push(it->second, ClassConvCost);
            return res;
        }
    }
    if (!canon_->is_prim())
        return res;

    // class to primitive type to other primitive type
    for (auto& [_, fun] : _convs) {
        auto ret_canon = fun->ret_type()->canon();
        if (!ret_canon->is_prim())
            continue;
        if (ret_canon->is_impl_castable_to(canon_)) {
            // implicit
            auto cost = prim_conv_cost(ret_canon->as_prim(), canon_->as_prim());
            res.push(fun, ClassConvCost + cost);
        } else if (allow_cast && ret_canon->is_expl_castable_to(canon_)) {
            // explicit
            auto cost = prim_cast_cost(ret_canon->as_prim(), canon_->as_prim());
            res.push(fun, ClassConvCost + cost);
        }
    }
    return res;
}

ConvList Class::convs(BuiltinTypeId bi_type_id, bool allow_cast) const {
    ConvList res;
    for (auto& [_, fun] : _convs) {
        auto ret_canon = fun->ret_type()->canon();
        if (!ret_canon->is_prim())
            continue;
        if (ret_canon->is_impl_castable_to(bi_type_id)) {
            // implicit
            auto cost = prim_conv_cost(ret_canon->as_prim(), bi_type_id);
            res.push(fun, ClassConvCost + cost);
        } else if (allow_cast && ret_canon->is_expl_castable_to(bi_type_id)) {
            // explicit
            auto cost = prim_cast_cost(ret_canon->as_prim(), bi_type_id);
            res.push(fun, ClassConvCost + cost);
        }
    }
    return res;
}

void Class::add_conv(Ref<Fun> fun) {
    assert(fun->cls() == this);
    auto ret_canon = fun->ret_type()->canon();
    assert(_convs.count(ret_canon->id()) == 0);
    _convs[ret_canon->id()] = fun;
}

} // namespace ulam
