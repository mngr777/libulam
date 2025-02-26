#include <algorithm>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/ast/nodes/params.hpp>
#include <libulam/diag.hpp>
#include <libulam/semantic/fun.hpp>
#include <libulam/semantic/mangler.hpp>
#include <libulam/semantic/scope/version.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/conv.hpp>
#include <libulam/semantic/var.hpp>

#define DEBUG_FUN // TEST
#ifdef DEBUG_FUN
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[ulam::Fun] "
#    include "src/debug.hpp"
#endif

namespace ulam {

// Fun

Fun::Fun(Ref<ast::FunDef> node): _node{node} {
    assert(!node->fun());
    node->set_fun(this);
}

Fun::~Fun() {}

str_id_t Fun::name_id() const { return _node->name_id(); }

bool Fun::is_native() const { return _node->is_native(); }

void Fun::add_param(Ptr<Var>&& param) {
    assert(params_node());
    assert(_params.size() < params_node()->child_num());
    _params.push_back(std::move(param));
}

void Fun::add_param(Ref<ast::Param> node) {
    auto param = make<Var>(node->type_name(), node, Ref<Type>{}, Var::FunParam);
    add_param(std::move(param));
}

unsigned Fun::min_param_num() const {
    unsigned num = param_num();
    for (auto it = _params.rbegin(); it != _params.rend(); ++it) {
        if (!(*it)->node()->has_default_value())
            break;
        --num;
    }
    return num;
}

Fun::MatchRes Fun::match(const TypedValueList& args) {
    if (min_param_num() > args.size() || args.size() > param_num())
        return {NoMatch, MaxConvCost};

    conv_cost_t max_conv_cost = 0;
    auto arg_it = args.begin();
    auto param_it = _params.begin();
    for (; arg_it != args.end(); ++arg_it, ++param_it) {
        auto arg_type = arg_it->type();
        auto param_type = (*param_it)->type();
        max_conv_cost =
            std::max(max_conv_cost, arg_type->conv_cost(param_type));
        if (max_conv_cost == MaxConvCost)
            return {NoMatch, MaxConvCost};
    }

    auto status = (max_conv_cost == 0) ? ExactMatch : IsMatch;
    return {status, max_conv_cost};
}

Ref<ast::FunRetType> Fun::ret_type_node() const {
    assert(_node->has_ret_type());
    return _node->ret_type();
}

Ref<ast::ParamList> Fun::params_node() const {
    assert(_node->has_params());
    return _node->params();
}

Ref<ast::FunDefBody> Fun::body_node() const { return _node->body(); }

std::string Fun::key() const {
    TypeList param_types;
    for (const auto& param : _params)
        param_types.push_back(param->type());
    Mangler mangler;
    return mangler.mangled(param_types);
}

// FunSet

FunSet::Iterator::Iterator(FunList::iterator it): _it{it} {}

FunSet::Iterator::reference_type FunSet::Iterator::operator*() {
    return _it->ref();
}
FunSet::Iterator::pointer_type FunSet::Iterator::operator->() {
    return _it->ref();
}

bool FunSet::Iterator::operator==(const Iterator& other) {
    return _it == other._it;
}

bool FunSet::Iterator::operator!=(const Iterator& other) {
    return !operator==(other);
}

FunSet::Iterator& FunSet::Iterator::operator++() {
    _it++;
    return *this;
}

FunSet::Matches FunSet::find_match(const TypedValueList& args) {
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

void FunSet::add(Ptr<Fun>&& fun) { _funs.push_back(std::move(fun)); }

void FunSet::add(Ref<Fun> fun) { _funs.push_back(fun); }

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

void FunSet::merge(Ref<FunSet> other) {
    assert(_map.has_value());
    for (auto fun : *other)
        _map.value().emplace(fun->key(), fun); // silently fail on duplicates
}

} // namespace ulam
