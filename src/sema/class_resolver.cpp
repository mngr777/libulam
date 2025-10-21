#include <libulam/sema/class_resolver.hpp>
#include <libulam/sema/eval/env.hpp>
#include <libulam/sema/resolver.hpp>
#include <libulam/semantic/decl.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/scope/view.hpp>
#include <libulam/semantic/type/class.hpp>

namespace ulam::sema {

bool ClassResolver::init() {
    switch (_cls.state()) {
    case Decl::Initializing:
    case Decl::Initialized:
    case Decl::Resolved:
        return true;
    case Decl::Unresolvable:
        return false;
    default:
        _cls.set_state(Decl::Initializing);
    }

    bool ok = resolve_params() && init_ancestors();
    _cls.set_state(ok ? Decl::Initialized : Decl::Unresolvable);
    return ok;
}

bool ClassResolver::resolve(bool full) {
    return do_resolve() && (!full || resolve_rest());
}

bool ClassResolver::do_resolve() {
    if (!init())
        return false;

    switch (_cls.state()) {
    case Decl::Resolved:
        return true;
    case Decl::Resolving:
        _cls.set_state(Decl::Unresolvable);
        return false;
    case Decl::Unresolvable:
        return false;
    default:
        assert(
            _cls.state() == Decl::NotResolved ||
            _cls.state() == Decl::Initialized);
        _cls.set_state(Decl::Resolving);
    }

    bool ok = resolve_ancestors() && resolve_props() && resolve_funs() &&
              check_bitsize();
    _cls.set_state(ok ? Decl::Resolved : Decl::Unresolvable);
    if (ok) {
        _cls.merge_fsets();
        _cls.init_layout();
        init_default_data();
    }
    return ok;
}

bool ClassResolver::resolve_rest() {
    bool ok = true;
    for (auto type_def : _cls.type_defs())
        ok = _resolver.resolve(type_def) && ok;
    for (auto var : _cls.consts())
        ok = _resolver.resolve(var) && ok;
    return ok;
}

bool ClassResolver::resolve_params() {
    for (auto param : _cls.params()) {
        if (!_resolver.resolve(param))
            return false;
    }
    return true;
}

bool ClassResolver::init_ancestors() {
    if (!do_init_ancestors())
        return false;

    // Add UrSelf base
    // TODO: refactoring, add base class option
    {
        auto name_id = program()->str_pool().put("UrSelf");
        auto sym = _cls.module()->scope()->get(name_id);
        if (sym && sym->is<UserType>()) {
            auto urself_type = sym->get<UserType>();
            if (urself_type->is_class()) {
                auto urself = urself_type->as_class();
                if (urself != &_cls)
                    _cls.add_ancestor(urself, {});
            }
        }
    }

    add_inherited_props();
    return true;
}

bool ClassResolver::resolve_ancestors() {
    for (auto anc : _cls.ancestors()) {
        if (!_resolver.resolve(anc->cls()))
            return false;
    }
    _cls._ancestry.init();
    return true;
}

bool ClassResolver::resolve_props() {
    for (auto prop : _cls.all_props()) {
        if (!_resolver.resolve(prop))
            return false;
    }
    return true;
}

bool ClassResolver::resolve_funs() {
    if (!_resolver.resolve(_cls.constructors()))
        return false;
    for (auto [_, fset] : _cls._fsets)
        if (!_resolver.resolve(fset))
            return false;
    for (auto [_, fset] : _cls._convs)
        if (!_resolver.resolve(fset))
            return false;
    for (auto& [_, fset] : _cls.ops())
        if (!_resolver.resolve(ref(fset)))
            return false;
    return true;
}

bool ClassResolver::check_bitsize() {
    if (_cls.required_bitsize() <= _cls.bitsize())
        return true;

    auto prop = _cls.first_prop_over_max_bitsize();
    if (prop) {
        auto message = std::string{"size limit of "} +
                       std::to_string(_cls.bitsize()) + " bits exceeded";
        diag().error(prop->node(), std::move(message));
        return false;
    }

    auto parent = _cls.first_parent_over_max_bitsize();
    assert(parent);
    diag().error(parent->node(), "size limit exceeded");
    return false;
}

void ClassResolver::init_default_data() {
    _cls._init_bits = Bits{_cls.bitsize()};

    // element ID
    if (_cls.is_element())
        _cls._init_bits.write(AtomEltIdOff, AtomEltIdSize, _cls._elt_id);

    TypeIdSet unions; // initialized Unions
    for (auto prop : _cls.all_props()) {
        auto cls = prop->cls();
        if (unions.count(cls->id()) > 0)
            continue;

        _resolver.init_default_value(prop);
        auto off = prop->data_off_in(&_cls);
        prop->type()->store(_cls._init_bits, off, prop->default_value());
        if (cls->is_union())
            unions.insert(cls->id());
    }
}

bool ClassResolver::do_init_ancestors() {
    if (!_cls.node()->has_ancestors())
        return true;

    auto ssr = env().scope_switch_raii(_cls.param_scope());
    for (unsigned n = 0; n < _cls.node()->ancestors()->child_num(); ++n) {
        auto cls_name = _cls.node()->ancestors()->get(n);
        auto anc = _resolver.resolve_class_name(cls_name, false);
        if (!anc)
            return false;
        _cls.add_ancestor(anc, cls_name);
    }
    return true;
}

void ClassResolver::add_inherited_props() {
    // parents first
    for (auto anc : _cls.parents()) {
        for (auto prop : anc->cls()->props())
            _cls._all_props.push_back(prop);
    }

    // grandparents
    for (auto anc : _cls.ancestors()) {
        if (anc->is_parent())
            continue;
        for (auto prop : anc->cls()->props())
            _cls._all_props.push_back(prop);
    }
}

} // namespace ulam::sema
