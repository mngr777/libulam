#include "libulam/semantic/decl.hpp"
#include "libulam/semantic/type/class/layout.hpp"
#include "libulam/semantic/value/types.hpp"
#include <algorithm>
#include <cassert>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/ast/nodes/type.hpp>
#include <libulam/ast/nodes/var_decl.hpp>
#include <libulam/sema/resolver.hpp>
#include <libulam/semantic/module.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/scope/iterator.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/class/prop.hpp>
#include <libulam/semantic/type/class_tpl.hpp>
#include <libulam/semantic/type/prim.hpp>
#include <libulam/semantic/value.hpp>

#define DEBUG_CLASS // TEST
#ifdef DEBUG_CLASS
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[ulam::Class] "
#    include "src/debug.hpp"
#endif

namespace ulam {

Class::Class(std::string_view name, Ref<ClassTpl> tpl):
    UserType{
        tpl->module()->program()->builtins(),
        &tpl->module()->program()->type_id_gen()},
    ClassBase{tpl->node(), tpl->module(), scp::Class},
    _name{name},
    _tpl{tpl} {}

Class::Class(
    std::string_view name, Ref<ast::ClassDef> node, Ref<Module> module):
    UserType{module->program()->builtins(), &module->program()->type_id_gen()},
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
    _props.push_back(prop);
    return prop;
}

bool Class::init(sema::Resolver& resolver) {
    switch (state()) {
    case Initializing:
    case Initialized:
    case Resolved:
        return true;
    case Unresolvable:
        return false;
    default:
        set_state(Initializing);
    }

    bool resolved = resolve_params(resolver) && init_ancestors(resolver, false);
    set_state(resolved ? Initialized : Unresolvable);
    return resolved;
}

bool Class::resolve(sema::Resolver& resolver) {
    switch (state()) {
    case Resolved:
        return true;
    case Resolving:
        set_state(Unresolvable);
        return false;
    case Unresolvable:
        return false;
    default:
        set_state(Resolving);
    }

    bool resolved = init_ancestors(resolver, true) && resolve_members(resolver);
    if (resolved) {
        merge_fsets();
        init_layout();
    }
    set_state(resolved ? Resolved : Unresolvable);
    return resolved;
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
        if (kind() == ClassKind::Union) {
            total = std::max(total, prop->bitsize());
        } else {
            total += prop->bitsize();
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
    BitVectorView data, BitVector::size_t off, const RValue& rval) const {
    assert(rval.is<SPtr<Object>>());
    data.write(off, rval.get<SPtr<Object>>()->bits().view());
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

Ref<FunSet> Class::add_fset(str_id_t name_id) {
    auto fset = ClassBase::add_fset(name_id);
    if (!fset->has_cls())
        fset->set_cls(this);
    return fset;
}

bool Class::resolve_params(sema::Resolver& resolver) {
    for (auto [_, sym] : *param_scope()) {
        auto scope_version = sym->as_decl()->scope_version();
        auto scope = param_scope()->view(scope_version);
        if (!resolver.resolve(sym, ref(scope)))
            return false;
    }
    return true;
}

bool Class::init_ancestors(sema::Resolver& resolver, bool resolve) {
    if (!node()->has_ancestors())
        return true;

    for (unsigned n = 0; n < node()->ancestors()->child_num(); ++n) {
        auto cls_name = node()->ancestors()->get(n);
        // TODO: check if element's ancestor is element or transient etc.
        auto cls =
            resolver.resolve_class_name(cls_name, param_scope(), resolve);
        if (!cls)
            return false;
        add_ancestor(cls, cls_name);
    }
    return true;
}

bool Class::resolve_members(sema::Resolver& resolver) {
    for (auto [_, sym] : *scope()) {
        auto scope_version = sym->as_decl()->scope_version();
        auto scope = param_scope()->view(scope_version);
        if (!resolver.resolve(sym, ref(scope)))
            return false;
    }
    return true;
}

void Class::add_ancestor(Ref<Class> cls, Ref<ast::TypeName> node) {
    if (!_ancestry.add(cls, node))
        return;
    for (auto& [name_id, sym] : cls->members()) {
        if (!sym.is<FunSet>()) // handled separately, see Class::merge_fsets
            members().import_sym(name_id, sym);
    }
}

void Class::merge_fsets() {
    for (auto parent : parents()) {
        for (auto [name_id, fset] : parent->cls()->fsets()) {
            auto sym = get(name_id);
            if (!sym || sym->is<FunSet>())
                add_fset(name_id)->merge(fset);
        }
    }
}

void Class::init_layout() {
    cls::data_off_t off = 0;

    // properties
    for (auto prop : props()) {
        prop->set_data_off(off);
        if (kind() == ClassKind::Union) {
            off = std::max<bitsize_t>(off, prop->bitsize());
        } else {
            off += prop->bitsize();
        }
    }

    // TODO: ancestors
}

} // namespace ulam
