#pragma once
#include <cassert>
#include <libulam/ast/visitor.hpp>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#define ULAM_AST_NODE                                                          \
public:                                                                        \
    virtual bool accept(Visitor& v) override { return v.visit(*this); }        \
    virtual bool accept(Visitor& v) const override { return v.visit(*this); }  \
                                                                               \
private:

namespace ulam::ast {

class Node;

// TODO: type constraints
template <typename N> using Ptr = std::unique_ptr<N>;

// TODO: pass context
template <typename N, typename... Args> Ptr<N> make(Args&&... args) {
    return std::make_unique<N>(std::forward<Args>(args)...);
}

template <typename... Ns> using Variant = std::variant<Ptr<Ns>...>;

template <typename... Ns> Node* as_node(Variant<Ns...>& v) {
    return std::visit([](auto&& ptr) -> Node* { return ptr.get(); }, v);
}

template <typename... Ns> const Node* as_node(const Variant<Ns...>& v) {
    return std::visit([](auto&& ptr) -> Node* { return ptr.get(); }, v);
}

class Node {
public:
    virtual ~Node();

    virtual bool accept(Visitor& visitor);
    virtual bool accept(Visitor& visitor) const;

    virtual unsigned child_num() const { return 0; }

    virtual Node* child(unsigned n) { return nullptr; }
    virtual const Node* child(unsigned n) const { return nullptr; }
};

template <typename... Ns> class Tuple : public Node {
private:
    using T = std::tuple<Ptr<Ns>...>;
    template <std::size_t I> using E = typename std::tuple_element<I, T>::type;

public:
    unsigned child_num() const override { return std::tuple_size<T>(); }

    Node* child(unsigned n) override { return getters()[n](*this); }
    const Node* child(unsigned n) const override { return getters()[n](*this); }

protected:
    template <std::size_t I> auto get() { return std::get<I>(_items).get(); }

    template <std::size_t I> const auto get() const {
        return std::get<I>(_items).get();
    }

    template <std::size_t I> E<I> replace(E<I>&& repl) {
        std::swap(std::get<I>(_items), repl);
        return repl;
    }

private:
    template <std::size_t I> static Node* item_as_node(Tuple& self) {
        return std::get<I>(self._items).get();
    }

    template <std::size_t... Indices>
    auto static getters(
        std::index_sequence<Indices...> seq =
            std::make_index_sequence<sizeof...(Ns)>()) {
        using Get = decltype(item_as_node<0>);
        static constexpr Get* table[] = {as_node<Indices>...};
        return std::decay(table);
    }

    T _items;
};

template <typename N> class List : public Node {
public:
    using Item = Ptr<N>;

    void add(Ptr<N>&& item) { _items.push_back(std::move(item)); }

    N* get(unsigned n) {
        assert(n < _items.size());
        return _items[n].get();
    }
    const N* get(unsigned n) const {
        assert(n < _items.size());
        return _items[n].get();
    }

    Item replace(unsigned n, Ptr<N>&& repl) {
        assert(n < _items.size());
        _items[n].swap(repl);
        return repl;
    }

    unsigned child_num() const override { return _items.size(); }

    Node* child(unsigned n) override { return _items[n].get(); }
    const Node* child(unsigned n) const override { return _items[n].get(); }

private:
    std::vector<Item> _items; // TODO: list?
};

template <typename... Ns> class ListOf : public Node {
public:
    using Item = Variant<Ns...>;

    template <typename N> void add(Ptr<N>&& item) {
        _items.push_back(std::move(item));
    }

    Item& get(unsigned n) {
        assert(n < _items.size());
        return _items[n];
    }
    const Item& get(unsigned n) const {
        assert(n < _items.size());
        return _items[n];
    }

    template <typename N> Item replace(unsigned n, Ptr<N>&& repl) {
        return replace(Item{repl});
    }
    template <typename N> Item replace(unsigned n, Item&& repl) {
        assert(n < _items.size());
        _items[n].swap(repl);
        return repl;
    }

    unsigned child_num() const override { return _items.size(); }

    Node* child(unsigned n) override { return as_node(_items[n]); }
    const Node* child(unsigned n) const override { return as_node(_items[n]); }

private:
    std::vector<Variant<Ns...>> _items; // TODO: list?
};

} // namespace ulam::ast
