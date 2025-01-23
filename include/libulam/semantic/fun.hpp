#pragma once
#include <functional>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/decl.hpp>
#include <libulam/semantic/typed_value.hpp>
#include <libulam/str_pool.hpp>
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
class FunSet;
class Scope;
class Type;
class Var;

class Fun : public Decl {
    friend FunSet;

public:
    enum Match { NoMatch, IsMatch, ExactMatch };

    Fun(Ref<ast::FunDef> node);
    ~Fun();

    str_id_t name_id() const;

    Ref<Type> ret_type() { return _ret_type; }
    Ref<const Type> ret_type() const { return _ret_type; }

    void set_ret_type(Ref<Type> ret_type) { _ret_type = ret_type; }

    // TMP
    auto& params() { return _params; }
    const auto& param() const { return _params; }
    void add_param(Ptr<Var>&& param);

    unsigned min_param_num() const;
    unsigned param_num() const { return _params.size(); }

    Match match(const TypedValueList& args);

    Ref<Scope> scope();

    Ref<ast::FunDef> node() { return _node; }
    Ref<ast::FunRetType> ret_type_node();
    Ref<ast::ParamList> params_node();
    Ref<ast::FunDefBody> body_node();

private:
    std::string key() const;

    Ref<ast::FunDef> _node;
    Ref<Type> _ret_type{};
    std::list<Ptr<Var>> _params;
};

class FunSet : public Decl {
public:
    using Cb = std::function<void(Ref<Fun>)>;
    using MatchRes = std::unordered_set<Ref<Fun>>;

    FunSet() {}
    FunSet(FunSet& other);

    MatchRes find_match(const TypedValueList& args);

    void for_each(Cb cb);

    void add(Ptr<Fun>&& fun);
    void add(Ref<Fun> fun);

    void init_map(Diag& diag, UniqStrPool& str_pool);

    void merge(Ref<FunSet> other);

private:
    using ParamTypeMap = std::unordered_map<std::string, Ref<Fun>>;

    std::list<RefPtr<Fun>> _funs;
    std::optional<ParamTypeMap> _map;
};

} // namespace ulam
