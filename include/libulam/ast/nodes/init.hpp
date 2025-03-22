#pragma once
#include <cassert>
#include <libulam/ast/node.hpp>
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/str_pool.hpp>
#include <list>
#include <map>

namespace ulam::ast {

class InitList;
class InitMap;
class InitValue;

class InitList : public ListOf<Node, Expr, InitList, InitMap> {
    ULAM_AST_NODE
    ULAM_AST_SIMPLE_ATTR(bool, is_constr_call, false)
public:
    bool empty() const { return size() == 0; }

    unsigned size() const { return child_num(); }
};

class InitMap : public InitList {
    ULAM_AST_NODE
public:
    using KeyList = std::list<str_id_t>;

    bool has(str_id_t name_id) const { return _map.count(name_id) > 0; }

    auto& get(str_id_t name_id) {
        assert(has(name_id));
        unsigned idx = _map[name_id];
        return ListOf::get(idx);
    }

    const auto& get(str_id_t name_id) const {
        return const_cast<InitMap*>(this)->get(name_id);
    }

    template <typename N> void add(str_id_t name_id, Ptr<N>&& item) {
        assert(_map.count(name_id) == 0);
        unsigned idx = child_num();
        ListOf::add(std::move(item));
        _keys.push_back(name_id);
        _map[name_id] = idx;
    }

    const KeyList keys() const { return _keys; }

private:
    KeyList _keys;
    std::map<str_id_t, unsigned> _map;
};

class InitValue : public Variant<Node, Expr, InitList, InitMap> {
    ULAM_AST_NODE
public:
    template <typename T>
    InitValue(T&& item): Variant{Variant::ItemT{std::forward<T>(item)}} {}

    bool is_constr_call() const {
        return get().accept(
            [&](const Ptr<ast::InitList>& list) { return list->is_constr_call(); },
            [&](auto&& other) { return false; });
    }
};

} // namespace ulam::ast
