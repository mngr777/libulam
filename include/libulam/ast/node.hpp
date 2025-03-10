#pragma once
#include "libulam/semantic/scope/version.hpp"
#include <cassert>
#include <libulam/ast/str.hpp>
#include <libulam/ast/visitor.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/src_loc.hpp>
#include <libulam/str_pool.hpp>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#define ULAM_AST_NODE                                                          \
public:                                                                        \
    virtual void accept(Visitor& v) override { v.visit(this); }                \
                                                                               \
private:

#define ULAM_AST_TUPLE_PROP(name, index)                                       \
    bool has_##name() const { return (bool)std::get<index>(_items); }          \
    auto name() { return ref(std::get<index>(_items)); }                       \
    const auto name() const { return ref(std::get<index>(_items)); }           \
    auto replace_##name(ElementT<index>&& repl) {                              \
        return replace<index>(std::move(repl));                                \
    }

#define ULAM_AST_SIMPLE_ATTR(type, name, init)                                 \
public:                                                                        \
    type name() const { return _attr_##name; }                                 \
    void set_##name(type value) { _attr_##name = value; }                      \
                                                                               \
private:                                                                       \
    type _attr_##name{init};

#define ULAM_AST_REF_ATTR(type, name)                                          \
public:                                                                        \
    ulam::Ref<type> name() { return _attr_##name; }                            \
    ulam::Ref<const type> name() const { return _attr_##name; }                \
    void set_##name(ulam::Ref<type> value) { _attr_##name = value; }           \
                                                                               \
private:                                                                       \
    Ref<type> _attr_##name{};

#define ULAM_AST_PTR_ATTR(type, name)                                          \
public:                                                                        \
    ulam::Ref<type> name() { return ulam::ref(_attr_##name); }                 \
    ulam::Ref<const type> name() const { return ulam::ref(_attr_##name); }     \
    void set_##name(ulam::Ptr<type>&& value) {                                 \
        std::swap(_attr_##name, value);                                        \
    }                                                                          \
                                                                               \
private:                                                                       \
    Ptr<type> _attr_##name{};

namespace ulam::ast {

// TODO:
// - child iterators

// Node base

class Node {
    ULAM_AST_SIMPLE_ATTR(loc_id_t, loc_id, NoLocId)
public:
    virtual ~Node();

    virtual void accept(Visitor& visitor);

    virtual unsigned child_num() const { return 0; }

    virtual Ref<Node> child(unsigned n) { return nullptr; }
    virtual Ref<const Node> child(unsigned n) const { return nullptr; }
};

// Named node trait
class Named {
    ULAM_AST_SIMPLE_ATTR(Str, name, Str{})
public:
    Named(Str name): _attr_name{name} {}

    str_id_t name_id() const { return name().str_id(); }
};

// Version number of persistent scope after definition
class DefNode {
    ULAM_AST_SIMPLE_ATTR(ScopeVersion, scope_version, NoScopeVersion)
public:
    bool has_scope_version() { return scope_version() != NoScopeVersion; }
};

template <typename N> class NameIdMap {
public:
    bool has(str_id_t name_id) const { return _map.count(name_id) == 1; }

    void add(str_id_t name_id, Ref<N> node) {
        assert(!has(name_id));
        _map[name_id] = node;
    }

    Ref<N> get(str_id_t name_id) {
        auto it = _map.find(name_id);
        return (it != _map.end()) ? it->second : Ref<N>{};
    }

    Ref<const N> get(str_id_t name_id) const {
        auto it = _map.find(name_id);
        return (it != _map.end()) ? it->second : Ref<N>{};
    }

private:
    std::unordered_map<str_id_t, Ref<N>> _map;
};

// Tuple

template <typename B, typename... Ns> class Tuple : public B {
public:
    using TupleT = std::tuple<Ptr<Ns>...>;

    template <std::size_t I>
    using ElementT = typename std::tuple_element<I, TupleT>::type;

    template <typename... BaseArgs>
    Tuple(Ptr<Ns>&&... items, BaseArgs&&... base_args):
        B{std::forward<BaseArgs>(base_args)...},
        _items(std::forward<Ptr<Ns>>(items)...) {}

    Tuple(Ptr<Ns>&&... items): _items(std::forward<Ptr<Ns>>(items)...) {}

    unsigned child_num() const override { return std::tuple_size<TupleT>(); }

    Ref<Node> child(unsigned n) override {
        auto node =
            getters(std::make_index_sequence<sizeof...(Ns)>())[n](*this);
        return const_cast<Node*>(node);
    }
    Ref<const Node> child(unsigned n) const override {
        return getters(std::make_index_sequence<sizeof...(Ns)>())[n](*this);
    }

protected:
    template <std::size_t I> auto get() { return ref(std::get<I>(_items)); }

    template <std::size_t I> const auto get() const {
        return ref(std::get<I>(_items));
    }

    template <std::size_t I> ElementT<I> replace(ElementT<I>&& repl) {
        std::swap(std::get<I>(_items), repl);
        return std::move(repl);
    }

    TupleT _items;

private:
    template <std::size_t I>
    static const Ref<Node> item_as_node_ref(const Tuple& self) {
        return ref(std::get<I>(self._items));
    }

    template <std::size_t... Indices>
    auto static getters(std::index_sequence<Indices...>) {
        using Get = decltype(item_as_node_ref<0>);
        static constexpr Get* table[] = {item_as_node_ref<Indices>...};
        return table;
    }
};

// List of nodes of same type

template <typename B, typename N> class List : public B {
public:
    using ItemT = Ptr<N>;

    void add(Ptr<N>&& item) { _items.push_back(std::move(item)); }

    Ref<N> get(unsigned n) {
        assert(n < _items.size());
        return ref(_items[n]);
    }
    const Ref<N> get(unsigned n) const {
        assert(n < _items.size());
        return ref(_items[n]);
    }

    ItemT replace(unsigned n, Ptr<N>&& repl) {
        assert(n < _items.size());
        _items[n].swap(repl);
        return std::move(repl);
    }

    unsigned child_num() const override { return _items.size(); }

    Ref<Node> child(unsigned n) override { return ref(_items[n]); }
    Ref<const Node> child(unsigned n) const override { return ref(_items[n]); }

private:
    std::vector<ItemT> _items; // TODO: list?
};

// List of nodes of multiple types

template <typename B, typename... Ns> class ListOf : public B {
public:
    using ItemT = Variant<Ns...>;

    template <typename N> void add(Ptr<N>&& item) {
        _items.push_back(std::move(item));
    }

    ItemT& get(unsigned n) {
        assert(n < _items.size());
        return _items[n];
    }
    const ItemT& get(unsigned n) const {
        assert(n < _items.size());
        return _items[n];
    }

    template <typename N> ItemT replace(unsigned n, Ptr<N>&& repl) {
        return replace(ItemT{repl});
    }
    template <typename N> ItemT replace(unsigned n, ItemT&& repl) {
        assert(n < _items.size());
        _items[n].swap(repl);
        return repl;
    }

    unsigned child_num() const override { return _items.size(); }

    Ref<Node> child(unsigned n) override {
        return const_cast<Ref<Node>>(std::as_const(*this).child(n));
    }

    Ref<const Node> child(unsigned n) const override {
        return std::visit(
            [](auto&& ptr) -> Ref<const Node> { return ref(ptr); }, _items[n]);
    }

private:
    std::vector<ItemT> _items; // TODO: list?
};

} // namespace ulam::ast
