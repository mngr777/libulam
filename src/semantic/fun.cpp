#include <algorithm>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/ast/nodes/params.hpp>
#include <libulam/diag.hpp>
#include <libulam/semantic/fun.hpp>
#include <libulam/semantic/mangler.hpp>
#include <libulam/semantic/scope/version.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/conv.hpp>
#include <libulam/semantic/var.hpp>

#ifdef DEBUG_FUN
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[ulam::Fun] "
#    include "src/debug.hpp"
#endif

namespace ulam {

// Fun

Fun::Fun(Mangler& mangler, Scope* scope, Ref<ast::FunDef> node):
    _mangler{mangler}, _param_scope{make<PersScope>(scope)}, _node{node} {
    assert(node);
    if (node->is_marked_virtual())
        set_is_virtual(true);
}

Fun::~Fun() {}

str_id_t Fun::name_id() const { return _node->name_id(); }

bool Fun::is_constructor() const { return _node->is_constructor(); }

bool Fun::is_op() const { return _node->is_op(); }

bool Fun::is_op_alias() const { return _node->is_op_alias(); }

Op Fun::op() const { return _node->op(); }

bool Fun::is_marked_virtual() const { return node()->is_marked_virtual(); }

bool Fun::is_pure_virtual() const { return is_virtual() && !_node->has_body(); }

bool Fun::is_native() const { return _node->is_native(); }

bool Fun::has_ellipsis() const { return _node->params()->has_ellipsis(); }

void Fun::add_param(Ptr<Var>&& param) {
    assert(params_node());
    assert(_params.size() < params_node()->child_num());
    _param_scope->set(param->name_id(), ref(param));
    _params.push_back(std::move(param));
}

void Fun::add_param(Ref<ast::Param> node) {
    auto flags = Var::FunParam;
    if (node->is_const())
        flags |= Var::Const;
    auto param = make<Var>(node->type_name(), node, Ref<Type>{}, flags);
    add_param(std::move(param));
}

unsigned Fun::min_param_num() const {
    unsigned num = param_num();
    for (auto it = _params.rbegin(); it != _params.rend(); ++it) {
        if (!(*it)->node()->has_init())
            break;
        --num;
    }
    return num;
}

Fun::MatchRes Fun::match(const TypedValueRefList& args) {
    if (min_param_num() > args.size())
        return {NoMatch, MaxConvCost};
    if (args.size() > param_num() && !has_ellipsis())
        return {NoMatch, MaxConvCost};

    conv_cost_t max_conv_cost = 0;
    auto arg_it = args.begin();
    auto param_it = _params.begin();
    for (; arg_it != args.end(); ++arg_it, ++param_it) {
        if (param_it == _params.end()) {
            assert(has_ellipsis());
            break;
        }
        auto& param = (*param_it);
        auto& arg = (*arg_it).get();

        auto param_type = param->type();
        auto arg_type = arg.type();

        if (param_type->is_ref()) {
            // rvalue or xvalue lvalue cannot bind to non-const reference param
            if (!param->is_const() && arg.value().is_tmp())
                return {NoMatch, MaxConvCost};
            // bindable, pretend arg is a reference
            arg_type = arg_type->ref_type();
        }

        max_conv_cost = std::max(
            max_conv_cost, arg_type->conv_cost(param_type, arg.value()));
        if (max_conv_cost == MaxConvCost)
            return {NoMatch, MaxConvCost};
    }

    auto status = (max_conv_cost == 0) ? ExactMatch : IsMatch;
    return {status, max_conv_cost};
}

Ref<Fun> Fun::find_override(Ref<const Class> cls) {
    auto it = _overrides.find(cls->id());
    return (it != _overrides.end()) ? it->second : Ref<Fun>{};
}

Ref<PersScope> Fun::scope() { return cls()->scope(); }

Ref<ast::FunRetType> Fun::ret_type_node() const {
    assert(_node->has_ret_type());
    return _node->ret_type();
}

Ref<ast::ParamList> Fun::params_node() const {
    assert(_node->has_params());
    return _node->params();
}

Ref<ast::FunDefBody> Fun::body_node() const { return _node->body(); }

void Fun::add_override(Ref<Fun> fun, Ref<Class> cls) {
    assert(is_virtual());        // must be already marked as virtual
    assert(fun->key() == key()); // parameters must match

    if (_overrides.count(cls->id()) == 1) {
        // can happen with multible inheritance
        assert(_overrides[cls->id()] == fun);
        return;
    }

    fun->set_is_virtual(true);
    if (!fun->has_overridden())
        fun->set_overridden(this);
    _overrides[cls->id()] = fun;

    if (has_overridden())
        overridden()->add_override(fun, cls);
}

bool Fun::has_overridden() const { return _overridden; }

Ref<Fun> Fun::overridden() {
    assert(_overridden);
    return _overridden;
}

void Fun::set_overridden(Ref<Fun> fun) {
    assert(!_overridden);
    _overridden = fun;
}

std::string Fun::key() const {
    TypeList param_types;
    for (const auto& param : _params)
        param_types.push_back(param->type());
    auto key = _mangler.mangled(param_types);
    if (has_ellipsis())
        key += "..."; // TMP
    return key;
}

// FunSet::Iter

FunSet::Iter::Iter(FunList::iterator it): _it{it} {}

FunSet::Iter::reference_type FunSet::Iter::operator*() { return _it->ref(); }
FunSet::Iter::pointer_type FunSet::Iter::operator->() { return _it->ref(); }

bool FunSet::Iter::operator==(const Iter& other) { return _it == other._it; }

bool FunSet::Iter::operator!=(const Iter& other) { return !operator==(other); }

FunSet::Iter& FunSet::Iter::operator++() {
    _it++;
    return *this;
}

// FunSet

FunSet::FunSet(str_id_t name_id): _name_id{name_id} {
    _map.emplace(); // empty map for empty set
}

bool FunSet::has_name_id() const { return _name_id != NoStrId; }

str_id_t FunSet::name_id() const {
    assert(has_name_id());
    return _name_id;
}

FunSet::Matches
FunSet::find_match(Ref<const Class> dyn_cls, const TypedValueRefList& args) {
    FunSet::Matches overrides{};
    auto matches = find_match(args);
    for (auto match : matches) {
        auto overrd = match->find_override(dyn_cls);
        overrides.insert(overrd ? overrd : match);
    }
    return overrides;
}

FunSet::Matches FunSet::find_match(const TypedValueRefList& args) {
    Matches matches;
    conv_cost_t min_conv_cost = MaxConvCost;
    for (auto fun : *this) {
        auto match_res = fun->match(args);
        switch (match_res.first) {
        case Fun::NoMatch:
            continue;
        case Fun::ExactMatch:
            return {fun};
        case Fun::IsMatch: {
            if (match_res.second < min_conv_cost) {
                // better match
                min_conv_cost = match_res.second;
                matches = {fun};
            } else if (match_res.second == min_conv_cost) {
                matches.insert(fun);
            }
        } break;
        }
    }
    return matches;
}

void FunSet::add(Ptr<Fun>&& fun) {
    _funs.push_back(std::move(fun));
    _map.reset();
}

void FunSet::add(Ref<Fun> fun) {
    _funs.push_back(fun);
    _map.reset();
}

void FunSet::init_map(Diag& diag, UniqStrPool& str_pool) {
    // debug() << __FUNCTION__ << "\n";
    assert(!_map.has_value());
    _map.emplace();

    // group funs by param types (hopefully one per group)
    using List = std::list<Ref<Fun>>;
    std::unordered_map<std::string, List> key_map;
    for (auto& item : _funs) {
        auto fun = item.ref();
        auto key = fun->key();
        // debug() << str_pool.get(fun->node()->name().str_id()) << " " << key
        // << "\n";
        auto [it, _] = key_map.emplace(key, List{});
        it->second.push_back(fun);
    }

    // for each group, if has single fun then add to map else complain
    for (auto& pair : key_map) {
        auto& [key, group] = pair;
        if (group.size() == 1) {
            _map.value().emplace(key, group.front());
        } else {
            for (auto fun : group) {
                auto name = fun->node()->name();
                diag.emit(
                    Diag::Error, name.loc_id(),
                    str_pool.get(name.str_id()).size(),
                    "multiple functions have same parameters");
            }
        }
    }
}

void FunSet::merge(Ref<FunSet> other, Ref<Class> cls, Ref<Class> from_cls) {
    assert(_map.has_value());
    for (auto fun : *other) {
        if (from_cls && fun->cls() != from_cls)
            continue;
        auto [it, added] = _map.value().emplace(fun->key(), fun);
        if (added) {
            _funs.push_back(fun);
            if (fun->is_virtual() && fun->has_overridden())
                fun->overridden()->add_override(fun, cls);

        } else if (fun->is_virtual() && !it->second->has_overridden()) {
            fun->add_override(it->second, cls);
        }
    }
}

} // namespace ulam
