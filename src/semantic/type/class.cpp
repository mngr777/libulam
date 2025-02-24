#include <cassert>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/semantic/module.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/class/prop.hpp>
#include <libulam/semantic/type/class_tpl.hpp>
#include <libulam/semantic/type/prim.hpp>

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
    _tpl{} {
    assert(!node->cls());
    node->set_cls(this);
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
