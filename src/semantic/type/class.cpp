#include "libulam/semantic/type/class/ancestry.hpp"
#include "libulam/semantic/type/conv.hpp"
#include <algorithm>
#include <cassert>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/ast/nodes/type.hpp>
#include <libulam/ast/nodes/var_decl.hpp>
#include <libulam/sema/resolver.hpp>
#include <libulam/semantic/decl.hpp>
#include <libulam/semantic/module.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/scope/iterator.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/builtin/atom.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/class/prop.hpp>
#include <libulam/semantic/type/class_tpl.hpp>
#include <libulam/semantic/type/prim.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/value/data.hpp>
#include <libulam/semantic/value/types.hpp>

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
    _tpl{tpl} {
    scope()->set_self_cls(this);
}

Class::Class(
    std::string_view name, Ref<ast::ClassDef> node, Ref<Module> module):
    UserType{module->program()->builtins(), &module->program()->type_id_gen()},
    ClassBase{node, module, scp::Class},
    _name{name},
    _tpl{} {
    scope()->set_self_cls(this);
}

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
    _all_props.push_back(prop);
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

    bool ok = resolve_params(resolver) && init_ancestors(resolver, false);
    set_state(ok ? Initialized : Unresolvable);
    return ok;
}

Ref<AliasType>
Class::init_type_def(sema::Resolver& resolver, str_id_t name_id) {
    auto sym = get(name_id);
    if (!sym)
        return {};
    auto alias = sym->get<UserType>()->as_alias();
    auto scope = alias->cls()->scope();
    auto scope_view = scope->view(alias->scope_version());
    resolver.resolve(alias, ref(scope_view));
    return alias;
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
        assert(state() == NotResolved || state() == Initialized);
        set_state(Resolving);
    }

    bool ok = resolve_params(resolver) && init_ancestors(resolver, true) &&
              resolve_props(resolver) && resolve_funs(resolver);
    if (ok) {
        merge_fsets();
        init_layout();
    }
    set_state(ok ? Resolved : Unresolvable);
    return ok;
}

Class::Symbol* Class::get_resolved(str_id_t name_id, sema::Resolver& resolver) {
    auto sym = get(name_id);
    assert(sym);
    auto is_resolved = sym->accept(
        [&](Ref<UserType> type) { return resolver.resolve(type->as_alias()); },
        [&](auto decl) { return resolver.resolve(decl); });
    if (!is_resolved)
        return {};
    return sym;
}

Ref<FunSet> Class::resolved_op(Op op, sema::Resolver& resolver) {
    auto fset = Class::op(op);
    assert(fset);
    if (!resolver.resolve(fset))
        return {};
    return fset;
}

bool Class::is_base_of(Ref<const Class> other) const {
    return other->_ancestry.is_base(this);
}

bool Class::is_same_or_base_of(Ref<const Class> other) const {
    return other == this || is_base_of(other);
}

Ref<Class> Class::base(str_id_t name_id) {
    auto anc = _ancestry.base(name_id);
    return anc ? anc->cls() : Ref<Class>{};
}

bool Class::has_super() const { return !_ancestry.parents().empty(); }

Ref<Class> Class::super() {
    assert(!_ancestry.parents().empty());
    return (*_ancestry.parents().begin())->cls();
}

bitsize_t Class::base_off(Ref<const Class> base) const {
    assert(base->is_base_of(this));
    return direct_bitsize() + _ancestry.data_off(base);
}

bitsize_t Class::bitsize() const {
    auto required = required_bitsize();
    switch (kind()) {
    case ClassKind::Element:
        return ULAM_ATOM_SIZE;
    case ClassKind::Transient:
        return required;
    default:
        return std::min<bitsize_t>(ULAM_ATOM_SIZE, required);
    }
}

bitsize_t Class::required_bitsize() const {
    auto size = direct_bitsize();
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

Ref<cls::Ancestor> Class::first_parent_over_max_bitsize() {
    if (is_transient())
        return {};
    bitsize_t size = direct_bitsize();
    for (auto parent : parents()) {
        size += parent->size_added();
        if (size > ULAM_ATOM_SIZE)
            return parent;
    }
    return {};
}

Ref<Prop> Class::first_prop_over_max_bitsize() {
    if (is_transient())
        return {};
    bitsize_t size = direct_bitsize();
    for (auto prop : _props) {
        size += prop->type()->bitsize();
        if (size > ULAM_ATOM_SIZE)
            return prop;
    }
    return {};
}

RValue Class::construct() const {
    // TMP?: const cast hack
    return RValue{make_s<Data>(const_cast<Class*>(this))};
}

RValue Class::load(const BitsView data, bitsize_t off) const {
    // TMP?: const cast hack
    return RValue{make_s<Data>(
        const_cast<Class*>(this), data.view(off, bitsize()).copy())};
}

void Class::store(BitsView data, bitsize_t off, const RValue& rval) const {
    assert(rval.is<DataPtr>());
    if (bitsize() > 0)
        data.write(off, rval.get<DataPtr>()->bits().view());
}

// NOTE: for ambiguous conversion truth is returned,
// the error is to be catched when applying conversions

bool Class::is_castable_to(Ref<const Type> type, bool expl) const {
    if (!convs(type, expl).empty())
        return true;
    return type->is_object() && is_castable_to_object_type(type, expl);
}

bool Class::is_castable_to(BuiltinTypeId bi_type_id, bool expl) const {
    return !convs(bi_type_id, expl).empty();
}

bool Class::is_castable_to_object_type(Ref<const Type> type, bool expl) const {
    assert(!is_same(type));
    assert(type->is_object());

    // to Atom
    if (type->is(AtomId))
        return is_element();

    assert(type->is_class());
    auto cls = type->as_class();

    // downcast
    if (cls->is_base_of(this))
        return true;

    // upcast
    return expl && is_base_of(cls);
}

Value Class::cast_to_object_type(Ref<const Type> type, Value&& val) const {
    assert(!is_same(type));

    auto rval = val.move_rvalue();
    assert(rval.is<DataPtr>());

    auto obj = rval.get<DataPtr>();
    assert(obj->type()->is_same(this));
    assert(is_castable_to_object_type(type, true));

    // to Atom
    if (type->is(AtomId)) {
        // TODO: const_cast
        auto atom = const_cast<Class*>(this)->builtins().atom_type()->construct(
            std::move(obj->bits()));
        return Value{std::move(atom)};
    }

    assert(type->is_class());
    auto cls = type->as_class();

    // downcast
    if (cls->is_base_of(this)) {
        auto new_rval = cls->construct();
        auto new_obj = new_rval.get<DataPtr>();
        for (auto prop : cls->all_props())
            prop->store(new_obj, prop->load(obj));
        return Value{std::move(new_rval)};
    }

    // upcast
    assert(is_base_of(cls));
    auto new_rval = cls->construct();
    auto new_obj = new_rval.get<DataPtr>();
    for (auto prop : all_props())
        prop->store(new_obj, prop->load(obj));
    return Value{std::move(new_rval)};
}

conv_cost_t Class::conv_cost(Ref<const Type> type, bool allow_cast) const {
    if (is_same(type))
        return 0;

    // user-defined conversions
    auto cost = convs(type, allow_cast).cost();
    if (cost != MaxConvCost)
        return cost;

    // class to Atom/class
    if (type->is_object() && is_castable_to_object_type(type, allow_cast)) {
        // to Atom
        if (type->is(AtomId))
            return ElementToAtomConvCost;
        assert(type->is_class());
        auto cls = type->as_class();
        // downcast
        if (cls->is_base_of(this))
            return ClassDowncastCost;
        // upcast
        assert(is_base_of(cls));
        return ClassUpcastCost;
    }
    return MaxConvCost;
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
    assert(fun->cls() == this); // TODO: inherited convs
    auto ret_canon = fun->ret_type()->canon();
    assert(_convs.count(ret_canon->id()) == 0);
    _convs[ret_canon->id()] = fun;
}

Ref<FunSet> Class::add_fset(str_id_t name_id) {
    auto fset = ClassBase::add_fset(name_id);
    fset->set_cls(this);
    assert(_fsets.count(name_id) == 0);
    _fsets[name_id] = fset;
    return fset;
}

Ref<FunSet> Class::add_op_fset(Op op) {
    auto fset = ClassBase::add_op_fset(op);
    fset->set_cls(this);
    return fset;
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
    if (resolve)
        _ancestry.init();
    return true;
}

bool Class::resolve_props(sema::Resolver& resolver) {
    for (auto prop : _all_props) {
        if (!resolver.resolve(prop))
            return false;
    }
    return true;
}

bool Class::resolve_funs(sema::Resolver& resolver) {
    for (auto [_, fset] : _fsets)
        if (!resolver.resolve(fset))
            return false;
    for (auto [_, fset] : _convs)
        if (!resolver.resolve(fset))
            return false;
    for (auto& [_, fset] : ops())
        if (!resolver.resolve(ref(fset)))
            return false;
    return true;
}

void Class::add_ancestor(Ref<Class> cls, Ref<ast::TypeName> node) {
    if (!_ancestry.add(cls, node))
        return;

    // import symbols
    for (auto& [name_id, sym] : cls->members()) {
        if (sym.is<FunSet>()) // handled separately, see Class::merge_fsets
            continue;
        if (members().import_sym(name_id, sym)) {
            auto name_id_ = name_id; // C++17 cannot capture struct-d bindings
            // TEST
            debug() << "importing " << cls->name() << "."
                    << module()->program()->str_pool().get(name_id) << " into "
                    << name() << "\n";
            sym.accept([&](auto mem) { inh_scope()->set(name_id_, mem); });
        }
    }

    // add inherited properties
    for (auto prop : cls->all_props())
        _all_props.push_back(prop);
}

void Class::merge_fsets() {
    for (auto parent : parents()) {
        // methods
        for (auto [name_id, fset] : parent->cls()->fsets()) {
            auto sym = get(name_id);
            if (!sym || sym->is<FunSet>())
                find_fset(name_id)->merge(fset);
        }
        // operators
        for (auto& [op, fset] : parent->cls()->ops())
            find_op_fset(op)->merge(ref(fset));
        // TODO: conversions
    }
}

void Class::init_layout() {
    bitsize_t off = 0;
    for (auto prop : props()) {
        prop->set_data_off(off);
        if (kind() == ClassKind::Union) {
            off = std::max<bitsize_t>(off, prop->bitsize());
        } else {
            off += prop->bitsize();
        }
    }
}

} // namespace ulam
