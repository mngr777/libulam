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
#include <libulam/semantic/scope/iter.hpp>
#include <libulam/semantic/type/builtin/atom.hpp>
#include <libulam/semantic/type/builtin/bits.hpp>
#include <libulam/semantic/type/builtin/unsigned.hpp>
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

Class::Class(Ref<ClassTpl> tpl):
    UserType{tpl->program()->builtins(), &tpl->program()->type_id_gen()},
    ClassBase{tpl->node(), tpl->module(), this},
    _tpl{tpl},
    _init_bits{0} {
    set_cls_tpl(tpl);
    init(tpl->module());
}

Class::Class(Ref<ast::ClassDef> node, Ref<Module> module):
    UserType{module->program()->builtins(), &module->program()->type_id_gen()},
    ClassBase{node, module, this},
    _tpl{},
    _init_bits{0} {
    init(module);
}

Class::~Class() {}

const std::string_view Class::name() const {
    return program()->str_pool().get(name_id());
}

str_id_t Class::name_id() const { return node()->name().str_id(); }

const std::string_view Class::mangled_name() const {
    if (_mangled_name.empty()) {
        _mangled_name = std::string{name()};
        if (!params().empty()) {
            Mangler& mangler = program()->mangler();
            TypeList param_types;
            for (const auto param : params())
                param_types.push_back(param->type());
            _mangled_name += "@" + mangler.mangled(param_types);
        }
    }
    return _mangled_name;
}

cls_id_t Class::class_id() const {
    if (_cls_id == NoClassId) {
        assert(program()->class_options().lazy_class_id);
        const_cast<Class*>(this)->register_class();
    }
    assert(_cls_id != NoClassId);
    return _cls_id;
}

elt_id_t Class::element_id() const {
    assert(is_element());
    assert(_elt_id != NoEltId);
    return _elt_id;
}

Ref<Var> Class::add_param(Ptr<Var>&& var) {
    assert(var->has_value());
    auto ref = ClassBase::add_param(std::move(var));
    ref->set_cls(this);
    return ref;
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

    // upcast/downcast
    assert(is_base_of(cls) || cls->is_base_of(this));
    auto props = is_base_of(cls) ? all_props() : cls->all_props();
    for (auto prop : props) {
        auto data_off = prop->data_off_in(this);
        prop->type()->store(data, off + data_off, prop->load(obj_data));
    }
}

TypedValue Class::type_op(TypeOp op) {
    switch (op) {
    case TypeOp::ClassIdOf: {
        auto type = builtins().unsigned_type();
        auto rval = type->construct(class_id());
        rval.set_is_consteval(true);
        return {type, Value{std::move(rval)}};
    }
    default:
        return UserType::type_op(op);
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

bool Class::is_castable_to(
    Ref<const Type> type, const Value& val, bool expl) const {
    if (is_same(type))
        return true;

    if (!val.empty()) {
        auto dyn_type = val.dyn_obj_type(true);
        if (!is_same(dyn_type))
            return dyn_type->is_castable_to(type, val, expl);
    }

    if (!convs(type, expl).empty())
        return true;

    // to Bits
    if (type->is(BitsId))
        return expl && (type->bitsize() == bitsize()); // too strict??

    // to Atom
    // NOTE: assuming element dyn type for unknown values, TODO: check if `type`
    // has element descendants
    if (type->is(AtomId))
        return is_element() || val.empty();

    if (!type->is_class())
        return false;

    auto cls = type->as_class();

    // downcast
    if (cls->is_base_of(this))
        return true;

    // upcast
    if (is_base_of(cls))
        return expl;

    // object of same size
    return expl && type->bitsize() == bitsize();
}

bool Class::is_castable_to(
    BuiltinTypeId bi_type_id, const Value& val, bool expl) const {
    // NOTE: must be conv or cast to exact type otherwise
    return !convs(bi_type_id, expl).empty();
}

bool Class::is_refable_as(
    Ref<const Type> type, const Value& val, bool expl) const {
    return is_castable_to(type, val, expl);
}

bool Class::is_assignable_to(Ref<const Type> type, const Value& val) const {
    if (is_same(type))
        return true;

    if (!val.empty() && type->is_object()) {
        auto dyn_type = val.dyn_obj_type(true);
        if (!is_same(dyn_type))
            return dyn_type->is_assignable_to(type, val);
    }

    // to Atom
    // NOTE: assuming element dyn type for unknown values, TODO: check if `type`
    // has element descendants
    if (type->is(AtomId))
        return is_element() || val.empty();

    if (!type->is_class())
        return false;

    auto cls = type->as_class();

    // element to element
    if (is_element() && cls->is_element())
        return true;

    // up/down cast
    if (cls->is_base_of(this) || is_base_of(cls))
        return true;

    return false;
}

Value Class::cast_to(Ref<Type> type, Value&& val) {
    if (is_same(type))
        return std::move(val);

    assert(convs(type, true).empty()); // must use conversion function otherwise

    if (!val.empty()) {
        auto dyn_type = val.dyn_obj_type();
        if (!is_same(dyn_type))
            return dyn_type->cast_to(type, std::move(val));
    }

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

    // upcast/downcast
    assert(is_base_of(cls) || cls->is_base_of(this));
    auto new_rval = cls->construct();
    auto new_obj = new_rval.get<DataPtr>();
    auto props = is_base_of(cls) ? all_props() : cls->all_props();
    for (auto prop : props)
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

void Class::init(Ref<Module> module) {
    set_module(module);

    if (!program()->class_options().lazy_class_id)
        register_class();

    if (is_element())
        register_element();
}

void Class::register_class() {
    assert(_cls_id == NoClassId);
    _cls_id = program()->classes().add(this);
}

void Class::register_element() {
    assert(is_element());
    assert(_elt_id == NoEltId);
    _elt_id = program()->elements().add(this);
}

elt_id_t Class::read_element_id(const BitsView data, bitsize_t off) {
    assert(is_element());
    return data.read(off + AtomEltIdOff, AtomEltIdSize);
}

void Class::add_ancestor(Ref<Class> cls, Ref<ast::TypeName> node) {
    bool added = node ? _ancestry.add(cls, node) : _ancestry.add_implicit(cls);
    if (!added)
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
                    << program()->str_pool().get(name_id) << " into " << name()
                    << "\n";
            sym.accept([&](auto mem) { inh_scope()->set(name_id_, mem); });
        }
    }
}

void Class::merge_fsets() {
    auto merge_methods_and_ops = [&](Ref<Class> other) {
        // methods
        for (auto [name_id, fset] : other->fsets()) {
            auto sym = get(name_id);
            if (!sym || sym->is<FunSet>())
                find_fset(name_id)->merge(fset, this, other);
        }

        // operators
        for (auto& [op, fset] : other->ops())
            find_op_fset(op)->merge(ref(fset), this, other);
    };

    // parents first
    for (auto parent : parents()) {
        auto other = parent->cls();

        // constructors
        constructors()->merge(other->constructors(), this);

        merge_methods_and_ops(other);

        // conversions
        for (auto [type_id, fun] : other->convs()) {
            if (_convs.count(type_id) == 0)
                add_conv(fun);
        }
    }

    // grandparents
    for (auto anc : ancestors()) {
        if (!anc->is_parent())
            merge_methods_and_ops(anc->cls());
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

Ref<Program> Class::program() const { return module()->program(); }

} // namespace ulam
