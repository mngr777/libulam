#include <libulam/ast/nodes/module.hpp>
#include <libulam/ast/nodes/params.hpp>
#include <libulam/diag.hpp>
#include <libulam/semantic/fun.hpp>
#include <libulam/semantic/mangler.hpp>
#include <libulam/semantic/scope/version.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/var.hpp>

#define ULAM_DEBUG
#define ULAM_DEBUG_PREFIX "[ulam::Fun] "
#include "src/debug.hpp"

namespace ulam {

// Fun

Fun::Fun(Ref<ast::FunDef> node): _node{node} {}
Fun::~Fun() {}

str_id_t Fun::name_id() const { return _node->name().str_id(); }

bool Fun::is_native() const { return _node->is_native(); }

Ref<ast::FunRetType> Fun::ret_type_node() {
    assert(_node->has_ret_type());
    return _node->ret_type();
}

void Fun::add_param(Ptr<Var>&& param) {
    assert(params_node());
    assert(_params.size() < params_node()->child_num());
    _params.push_back(std::move(param));
}

unsigned Fun::min_param_num() const {
    unsigned num = param_num();
    for (auto it = _params.rbegin(); it != _params.rend(); ++it) {
        if ((*it)->node()->has_default_value())
            break;
        --num;
    }
    return num;
}

Fun::Match Fun::match(const TypedValueList& args) {
    if (min_param_num() > args.size() || args.size() > param_num())
        return NoMatch;

    bool is_exact = true;
    auto arg_it = args.begin();
    auto param_it = _params.begin();
    for (; arg_it != args.end(); ++arg_it, ++param_it) {
        auto arg_type = arg_it->type();
        auto param_type = (*param_it)->type();
        if (*arg_type == *param_type)
            continue;
        if (!param_type->is_impl_castable(arg_type))
            return NoMatch;
        is_exact = false;
    }
    return is_exact ? ExactMatch : IsMatch;
}

Ref<ast::ParamList> Fun::params_node() { return _node->params(); }

Ref<ast::FunDefBody> Fun::body_node() { return _node->body(); }

std::string Fun::key() const {
    TypeList param_types;
    for (const auto& param : _params)
        param_types.push_back(param->type());
    Mangler mangler;
    return mangler.mangled(param_types);
}

// FunSet

FunSet::FunSet(FunSet& other) {
    other.for_each([&](Ref<Fun> fun) { add(fun); });
}

FunSet::MatchRes FunSet::find_match(const TypedValueList& args) {
    MatchRes res;
    for (auto& item : _funs) {
        auto fun = item.ref();
        switch (fun->match(args)) {
        case Fun::NoMatch:
            continue;
        case Fun::ExactMatch:
            return {fun};
        case Fun::IsMatch:
            res.insert(fun);
        }
    }
    return res;
}

void FunSet::for_each(Cb cb) {
    for (auto& item : _funs)
        cb(item.ref());
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
    other->for_each([&](Ref<Fun> fun) {
        _map.value().emplace(fun->key(), fun); // silently fail on duplicates
    });
}

} // namespace ulam
