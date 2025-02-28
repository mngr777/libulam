#pragma once
#include <iterator>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/decl.hpp>
#include <libulam/semantic/type/conv.hpp>
#include <libulam/semantic/typed_value.hpp>
#include <libulam/semantic/ops.hpp>
#include <libulam/str_pool.hpp>
#include <list>
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

class Diag;
class FunSet;
class Scope;
class Type;
class Var;

class Fun : public Decl {
    friend FunSet;

public:
    using Params = std::list<Ptr<Var>>;
    enum MatchStatus { NoMatch, IsMatch, ExactMatch };
    using MatchRes = std::pair<MatchStatus, conv_cost_t>;

    Fun(Ref<ast::FunDef> node);
    ~Fun();

    str_id_t name_id() const;

    bool is_op() const;
    Op op() const;

    bool is_native() const;

    Ref<Type> ret_type() { return _ret_type; }
    Ref<const Type> ret_type() const { return _ret_type; }

    void set_ret_type(Ref<Type> ret_type) { _ret_type = ret_type; }

    // TMP?
    Params& params() { return _params; }
    const Params& param() const { return _params; }
    void add_param(Ptr<Var>&& param); // TODO: remove
    void add_param(Ref<ast::Param> node);

    unsigned min_param_num() const;
    unsigned param_num() const { return _params.size(); }

    MatchRes match(const TypedValueList& args);

    Ref<Scope> scope();

    Ref<ast::FunDef> node() const { return _node; }
    Ref<ast::FunRetType> ret_type_node() const;
    Ref<ast::ParamList> params_node() const;
    Ref<ast::FunDefBody> body_node() const;

private:
    std::string key() const;

    Ref<ast::FunDef> _node;
    Ref<Type> _ret_type{};
    Params _params;
};

class FunSet : public Decl {
private:
    using FunList = std::list<RefPtr<Fun>>;

public:
    using Matches = std::unordered_set<Ref<Fun>>;

    class Iterator {
        friend FunSet;

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = Ref<Fun>;
        using pointer_type = Ref<Fun>;
        using reference_type = Ref<Fun>;

        Iterator& operator++();

        reference_type operator*();
        pointer_type operator->();

        bool operator==(const Iterator& other);
        bool operator!=(const Iterator& other);

    private:
        explicit Iterator(FunList::iterator it);

        FunList::iterator _it;
    };

    FunSet() {}
    FunSet(FunSet& other);

    bool is_virtual() const { return false; } // TODO

    Matches find_match(const TypedValueList& args);

    void add(Ptr<Fun>&& fun);
    void add(Ref<Fun> fun);

    Iterator begin() { return Iterator{_funs.begin()}; }
    Iterator end() { return Iterator{_funs.end()}; }

    std::size_t size() const { return _funs.size(); }
    bool empty() const { return _funs.empty(); }

    void init_map(Diag& diag, UniqStrPool& str_pool);

    void merge(Ref<FunSet> other);

private:
    using ParamTypeMap = std::unordered_map<std::string, Ref<Fun>>;

    FunList _funs;
    std::optional<ParamTypeMap> _map;
};

} // namespace ulam
