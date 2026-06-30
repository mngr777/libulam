#pragma once
#include <forward_list>
#include <iterator>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/def.hpp>
#include <libulam/semantic/ops.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/conv.hpp>
#include <libulam/semantic/typed_value.hpp>
#include <libulam/str_pool.hpp>
#include <list>
#include <map>
#include <optional>
#include <unordered_map>
#include <unordered_set>

namespace ulam::ast {
class FunDef;
class FunDefBody;
class FunRetType;
class Param;
class ParamList;
} // namespace ulam::ast

namespace ulam {

class Class;
class Diag;
class FunSet;
class Mangler;
class PersScope;
class Scope;
class Type;
class Var;

class Fun : public Def {
    friend FunSet;

public:
    using Params = std::list<Ptr<Var>>; // TODO: add list of refs
    enum MatchStatus { NoMatch, IsMatch, ExactMatch };
    using MatchRes = std::pair<MatchStatus, conv_cost_t>;

    Fun(UniqStrPool& str_pool,
        Mangler& mangler,
        Scope* scope,
        Ref<ast::FunDef> node);
    ~Fun();

    str_id_t name_id() const;
    const std::string_view name() const;

    const std::string_view mangled_name() const;

    bool is_constructor() const;

    bool is_op() const;
    bool is_op_alias() const;
    Op op() const;

    bool is_virtual() const { return _is_virtual; }
    void set_is_virtual(bool is_virtual) { _is_virtual = is_virtual; }

    bool is_pure_virtual() const;

    bool is_marked_virtual() const;

    bool is_native() const;

    bool has_ellipsis() const;

    Ref<Type> ret_type() { return _ret_type; }
    Ref<const Type> ret_type() const { return _ret_type; }

    void set_ret_type(Ref<Type> ret_type) { _ret_type = ret_type; }

    Params& params() { return _params; }
    const Params& param() const { return _params; }

    void add_param(Ref<ast::Param> node);

    unsigned min_param_num() const;
    unsigned param_num() const { return _params.size(); }

    MatchRes match(const TypedValueRefList& args);

    Ref<Fun> find_override(Ref<const Class> cls);

    Ref<PersScope> scope();
    Ref<PersScope> param_scope() { return ref(_param_scope); }

    Ref<ast::FunDef> node() const { return _node; }
    Ref<ast::FunRetType> ret_type_node() const;
    Ref<ast::ParamList> params_node() const;
    Ref<ast::FunDefBody> body_node() const;

private:
    void add_override(Ref<Fun> fun, Ref<Class> cls);

    bool has_overridden() const;
    Ref<Fun> overridden();
    void set_overridden(Ref<Fun> fun);

    std::string mangled_param_types() const;

    UniqStrPool& _str_pool;
    Mangler& _mangler;
    Ptr<PersScope> _param_scope;
    Ref<ast::FunDef> _node;
    Ref<Type> _ret_type{};
    Params _params{};
    bool _is_virtual{false};
    Ref<Fun> _overridden{};
    std::map<type_id_t, Ref<Fun>> _overrides;
    mutable std::string _mangled_name;
};

class FunSet : public Def {
private:
    using FunList = std::list<Ref<Fun>>;

public:
    using Matches = std::unordered_set<Ref<Fun>>;

    FunSet(str_id_t name_id = NoStrId);

    FunSet(FunSet&&) = default;
    FunSet& operator=(FunSet&&) = default;

    bool has_name_id() const;
    str_id_t name_id() const;

    Matches find_match(Ref<const Class> dyn_cls, const TypedValueRefList& args);
    Matches find_match(const TypedValueRefList& args);

    void add(Ptr<Fun>&& fun);
    void add(Ref<Fun> fun);

    auto begin() { return _funs.begin(); }
    auto end() { return _funs.end(); }

    std::size_t size() const { return _funs.size(); }
    bool empty() const { return _funs.empty(); }

    void init_map(Diag& diag, UniqStrPool& str_pool);

    void merge(Ref<FunSet> other, Ref<Class> cls, Ref<Class> from_cls = {});

private:
    using ParamTypeMap = std::unordered_map<std::string, Ref<Fun>>;

    str_id_t _name_id;
    FunList _funs;
    std::forward_list<Ptr<Fun>> _defs;
    std::optional<ParamTypeMap> _map;
};

} // namespace ulam
