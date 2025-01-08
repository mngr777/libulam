#pragma once
#include <functional>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/decl.hpp>
#include <libulam/semantic/params.hpp>
#include <list>
#include <optional>
#include <unordered_map>
#include <unordered_set>

namespace ulam::ast {
class FunDef;
class FunDefBody;
class FunRetType;
class ParamList;
} // namespace ulam::ast

namespace ulam {

class Diag;
class Type;

class Fun : public Decl {
public:
    enum Match { NoMatch, IsMatch, ExactMatch };

    Fun(Ref<ast::FunDef> node): _node{node} {}

    Ref<Type> ret_type() { return _ret_type; }
    Ref<const Type> ret_type() const { return _ret_type; }

    void set_ret_type(Ref<Type> ret_type) { _ret_type = ret_type; }

    // TMP
    auto& params() { return _params; }
    const auto& param() const { return _params; }
    void add_param(Ref<Type> type, Value&& default_value);

    unsigned min_param_num() const;
    unsigned param_num() const { return _params.size(); }

    Match match(const TypedValueList& args);

    Ref<ast::FunDef> node() { return _node; }
    Ref<ast::FunRetType> ret_type_node();
    Ref<ast::ParamList> params_node();
    Ref<ast::FunDefBody> body_node();

private:
    Ref<ast::FunDef> _node;
    Ref<Type> _ret_type{};
    ParamList _params;
};

class FunSet : public Decl {
public:
    using Cb = std::function<void(Ref<Fun>)>;
    using MatchRes = std::unordered_set<Ref<Fun>>;

    FunSet() {}
    FunSet(FunSet& other);

    MatchRes find_match(const TypedValueList& args);

    void for_each(Cb cb);

    Ref<Fun> add(Ref<ast::FunDef> node, ScopeVersion scope_version);

    void init_map(Diag& diag, UniqStrPool& str_pool);

    void merge(Ref<FunSet> other);

private:
    using ParamTypeMap = std::unordered_map<std::string, Ref<Fun>>;

    std::list<RefPtr<Fun>> _funs;
    std::optional<ParamTypeMap> _map;
};

} // namespace ulam
