#include "libulam/semantic/value/types.hpp"
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
#include <libulam/semantic/type/builtin/atom.hpp>
#include <libulam/semantic/type/builtin/bits.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/class/prop.hpp>
#include <libulam/semantic/type/class_tpl.hpp>
#include <libulam/semantic/type/prim.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/value/data.hpp>

#ifdef DEBUG_CLASS
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[ulam::Class] "
#endif
#include "src/debug.hpp"

namespace ulam {

Class::Class(std::string_view name, Ref<ClassTpl> tpl):
    UserType{tpl->program()->builtins(), &tpl->program()->type_id_gen()},
    ClassBase{tpl->node(), tpl->module(), scp::Class},
    _name{name},
    _tpl{tpl},
    _init_bits{0} {
    scope()->set_self_cls(this);
    set_module(tpl->module());
    if (is_element())
        register_element(tpl->program());
}

Class::Class(
    std::string_view name, Ref<ast::ClassDef> node, Ref<Module> module):
    UserType{module->program()->builtins(), &module->program()->type_id_gen()},
    ClassBase{node, module, scp::Class},
    _name{name},
    _tpl{},
    _init_bits{0} {
    scope()->set_self_cls(this);
    set_module(module);
    if (is_element())
        register_element(module->program());
}

Class::~Class() {}

str_id_t Class::name_id() const { return node()->name().str_id(); }

elt_id_t Class::element_id() const {
    assert(is_element());
    assert(_elt_id != NoEltId);
    return _elt_id;
}

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
    _type_defs.push_back(type);
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
    _consts.push_back(var);
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
    set_state(ok ? Resolved : Unresolvable);

    if (ok) {
        merge_fsets();
        init_layout();
        init_default_data(resolver);
    }
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

bool Class::has_super() const { return !_ancestry.parents().empty(); }

Ref<Class> Class::super() {
    assert(!_ancestry.parents().empty());
    return (*_ancestry.parents().begin())->cls();
}

Ref<Class> Class::base_by_name_id(str_id_t name_id) {
    auto anc = _ancestry.base(name_id);
    return anc ? anc->cls() : Ref<Class>{};
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
        return std::min<bitsize_t>(AtomDataMaxSize, required);
    }
}

bitsize_t Class::data_bitsize() const {
    assert(bitsize() >= data_off());
    return bitsize() - data_off();
}

bitsize_t Class::required_bitsize() const {
    auto size = direct_bitsize();
    for (const auto& anc : _ancestry.ancestors())
        size += anc->cls()->direct_bitsize();
    return size;
}

bitsize_t Class::direct_bitsize() const {
    bitsize_t total = data_off();
    for (auto prop : props()) {
        if (is_union()) {
            total = std::max(total, prop->bitsize());
        } else {
            total += prop->bitsize();
        }
    }
    return total;
}

bitsize_t Class::max_bitsize() const {
    return is_element() ? ULAM_ATOM_SIZE : AtomDataMaxSize;
}

bitsize_t Class::data_off() const { return is_element() ? AtomDataOff : 0; }

Ref<cls::Ancestor> Class::first_parent_over_max_bitsize() {
    if (is_transient())
        return {};
    bitsize_t size = direct_bitsize();
    for (auto parent : parents()) {
        size += parent->size_added();
        if (size > max_bitsize())
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
        if (size > max_bitsize())
            return prop;
    }
    return {};
}

RValue Class::construct() {
    return RValue{make_s<Data>(this, _init_bits.copy())};
}

RValue Class::construct(Bits&& bits) {
    assert(bits.len() == bitsize());
    return RValue{make_s<Data>(this, std::move(bits))};
}

RValue Class::load(const BitsView data, bitsize_t off) {
    assert(!is_element() || read_element_id(data, off) == _elt_id);
    return RValue{make_s<Data>(this, data.view(off, bitsize()).copy())};
}

void Class::store(BitsView data, bitsize_t off, const RValue& rval) {
    assert(rval.is<DataPtr>());
    auto obj_data = rval.get<DataPtr>();
    auto type = obj_data->type();

    // element data to Atom
    if (type->is(AtomId)) {
        assert(is_element());
        data.write(off, obj_data->bits().view());
        return;
    }

    // not Atom, must be a class
    assert(type->is_class());
    auto cls = obj_data->type()->as_class();

    // same class
    if (is_same(cls)) {
        if (bitsize() > 0)
            data.write(off, obj_data->bits().view());
        return;
    }

    // element data to other element (via Atom&, see t3986)
    if (is_element() && cls->is_element()) {
        data.write(
            off + AtomDataOff,
            obj_data->bits().view(data_off(), data_bitsize()));
        return;
    }

    if (is_base_of(cls)) {
        // base to derived
        for (auto prop : all_props()) {
            auto prop_off = prop->data_off();
            prop->type()->store(data, off + prop_off, prop->load(obj_data));
        }
    } else {
        // derived to base:
        // e.g. `Base a = self` where `Self` is Base and dynamic class of `self`
        // is derived from Base
        assert(cls->is_base_of(this));
        for (auto prop : cls->all_props()) {
            auto prop_off = prop->data_off_in(this);
            prop->type()->store(data, off + prop_off, prop->load(obj_data));
        }
    }
}

Ref<Type> Class::common(Ref<Type> type) {
    type = type->deref();
    if (is_same(type))
        return this;
    // use user-define conversion, t41140
    if (!convs(type, false).empty())
        return type;
    return this;
}

Ref<Type> Class::common(const Value& val1, Ref<Type> type, const Value& val2) {
    return common(type);
}

// NOTE: for ambiguous conversion truth is returned,
// the error is to be catched when applying conversions

bool Class::is_castable_to(Ref<const Type> type, bool expl) const {
    assert(!is_same(type));
    if (!convs(type, expl).empty())
        return true;

    // to Bits
    if (type->is(BitsId))
        return expl && (type->bitsize() == bitsize()); // too strict??

    // to Atom
    if (type->is(AtomId))
        return is_element();

    if (!type->is_class())
        return false;

    auto cls = type->as_class();

    // downcast
    if (cls->is_base_of(this))
        return true;

    // upcast
    if (is_base_of(cls))
        return expl && is_base_of(cls);

    // object of same size
    return expl && type->bitsize() == bitsize();
}

bool Class::is_castable_to(BuiltinTypeId bi_type_id, bool expl) const {
    // NOTE: must be conv or cast to exact type otherwise
    return !convs(bi_type_id, expl).empty();
}

Value Class::cast_to(Ref<Type> type, Value&& val) {
    assert(!is_same(type));
    assert(convs(type, true).empty()); // must use conversion function otherwise

    auto rval = val.move_rvalue();
    if (rval.empty())
        return Value{RValue{}};
    assert(rval.is<DataPtr>());

    bool is_consteval = rval.is_consteval();

    auto obj = rval.get<DataPtr>();
    assert(obj->type()->is_same(this));

    // to Bits
    if (type->is(BitsId)) {
        auto bits_type = builtins().bits_type(type->bitsize());
        auto new_rval = bits_type->construct(std::move(obj->bits()));
        new_rval.set_is_consteval(is_consteval);
        return Value{std::move(new_rval)};
    }

    // to Atom
    if (type->is(AtomId)) {
        assert(is_element());
        return Value{std::move(rval)};
    }

    assert(type->is_class());
    auto cls = type->as_class();

    // downcast
    if (cls->is_base_of(this))
        return Value{std::move(rval)};

    // upcast
    assert(is_base_of(cls));
    auto new_rval = cls->construct();
    auto new_obj = new_rval.get<DataPtr>();
    for (auto prop : all_props())
        prop->store(new_obj, prop->load(obj));
    new_rval.set_is_consteval(is_consteval);
    return Value{std::move(new_rval)};
}

conv_cost_t Class::conv_cost(Ref<const Type> type, bool allow_cast) const {
    if (is_same(type))
        return 0;

    // user-defined conversions
    auto cost = convs(type, allow_cast).cost();
    if (cost != MaxConvCost)
        return cost;

    if (!is_castable_to(type, allow_cast))
        return MaxConvCost;

    // to Bits
    if (type->is(BitsId))
        return ClassToBitsConvCost;

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

void Class::register_element(Ref<Program> program) {
    assert(is_element());
    assert(_elt_id == NoEltId);
    _elt_id = program->elements().add(this);
}

elt_id_t Class::read_element_id(const BitsView data, bitsize_t off) {
    return data.read(off + AtomEltIdOff, AtomEltIdSize);
}

bool Class::resolve_params(sema::Resolver& resolver) {
    auto scope = param_scope();
    for (auto param : params()) {
        auto scope_view = scope->view(param->scope_version());
        if (!resolver.resolve(param, ref(scope_view)))
            return false;
    }
    return true;
}

bool Class::init_ancestors(sema::Resolver& resolver, bool resolve) {
    if (node()->has_ancestors()) {
        for (unsigned n = 0; n < node()->ancestors()->child_num(); ++n) {
            auto cls_name = node()->ancestors()->get(n);
            // TODO: check if element's ancestor is element or transient etc.
            auto cls =
                resolver.resolve_class_name(cls_name, param_scope(), resolve);
            if (!cls)
                return false;
            add_ancestor(cls, cls_name);
        }
    }

    // add inherited properties
    for (auto anc : _ancestry.ancestors()) {
        for (auto prop : anc->cls()->props())
            _all_props.push_back(prop);
    }

    // TODO: refactoring
    {
        auto name_id = module()->program()->str_pool().put("UrSelf");
        auto sym = module()->scope()->get(name_id);
        if (sym && sym->is<UserType>()) {
            auto type = sym->get<UserType>();
            if (type->is_class()) {
                auto cls = type->as_class();
                if (cls != this) {
                    add_ancestor(cls, {});
                    if (resolve)
                        resolver.resolve(cls);
                }
            }
        }
    }

    if (resolve)
        _ancestry.init();
    return true;
}

bool Class::resolve_props(sema::Resolver& resolver) {
    for (auto prop : all_props()) {
        if (!resolver.resolve(prop))
            return false;
    }
    return true;
}

bool Class::resolve_funs(sema::Resolver& resolver) {
    if (!resolver.resolve(constructors()))
        return false;
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

void Class::init_default_data(sema::Resolver& resolver) {
    _init_bits = Bits(bitsize());

    // element ID
    if (is_element())
        _init_bits.write(AtomEltIdOff, AtomEltIdSize, _elt_id);

    TypeIdSet unions; // initialized Unions
    for (auto prop : all_props()) {
        auto cls = prop->cls();
        if (unions.count(cls->id()) > 0)
            continue;

        resolver.init_default_value(prop);
        auto off = prop->data_off_in(this);
        prop->type()->store(_init_bits, off, prop->default_value());
        if (cls->is_union())
            unions.insert(cls->id());
    }
}

void Class::add_ancestor(Ref<Class> cls, Ref<ast::TypeName> node) {
    if (!_ancestry.add(cls, node))
        return;

    // import ancestor types
    for (auto anc : cls->_ancestry.ancestors()) {
        auto name_id = anc->cls()->name_id();
        if (!inh_scope()->has(name_id))
            inh_scope()->set(name_id, anc->cls());
    }

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
}

void Class::merge_fsets() {
    for (auto parent : parents()) {
        // constructors
        constructors()->merge(parent->cls()->constructors(), this);

        // methods
        for (auto [name_id, fset] : parent->cls()->fsets()) {
            auto sym = get(name_id);
            if (!sym || sym->is<FunSet>())
                find_fset(name_id)->merge(fset, this);
        }

        // operators
        for (auto& [op, fset] : parent->cls()->ops())
            find_op_fset(op)->merge(ref(fset), this);

        // conversions
        for (auto [type_id, fun] : parent->cls()->convs()) {
            if (_convs.count(type_id) == 0)
                add_conv(fun);
        }
    }
}

void Class::init_layout() {
    bitsize_t off = data_off();
    for (auto prop : props()) {
        prop->set_data_off(off);
        if (!is_union())
            off += prop->bitsize();
    }
}

} // namespace ulam
