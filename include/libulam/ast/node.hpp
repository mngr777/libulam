#pragma once
#include <cassert>
#include <libulam/ast/visitor.hpp>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

#define ULAM_AST_NODE                                                          \
public:                                                                        \
    virtual bool accept(Visitor& v) override { return v.visit(*this); }        \
    virtual bool accept(Visitor& v) const override { return v.visit(*this); }  \
                                                                               \
private:

#define ULAM_AST_TUPLE_PROP(name, index)                                       \
    auto name() { return ref(std::get<index>(_items)); }                  \
    const auto name() const { return ref(std::get<index>(_items)); }      \
    auto replace_##name(ElementT<index>&& repl) {                              \
        return replace<index>(std::move(repl));                                \
    }

namespace ulam::ast {

class Node;

// TODO: type constraints
template <typename N> using Ptr = std::unique_ptr<N>; // owning pointer
template <typename N> using Ref = N*;                 // nullable reference

template <typename N> Ref<N> ref(Ptr<N>& ptr) { return ptr.get(); }
template <typename N> const Ref<N> ref(const Ptr<N>& ptr) { return ptr.get(); }

// TODO: pass context
template <typename N, typename... Args> Ptr<N> make(Args&&... args) {
    return std::make_unique<N>(std::forward<Args>(args)...);
}

template <typename... Ns> using Variant = std::variant<Ptr<Ns>...>;

template <typename... Ns> Ref<Node> as_node(Variant<Ns...>& v) {
    return std::visit([](auto&& ptr) -> Node* { return ref(ptr); }, v);
}

template <typename... Ns> const Ref<Node> as_node(const Variant<Ns...>& v) {
    return std::visit([](auto&& ptr) -> Node* { return ref(ptr); }, v);
}

class Node {
public:
    virtual ~Node();

    virtual bool accept(Visitor& visitor);
    virtual bool accept(Visitor& visitor) const;

    virtual unsigned child_num() const { return 0; }

    virtual Ref<Node> child(unsigned n) { return nullptr; }
    virtual const Ref<Node> child(unsigned n) const { return nullptr; }
};

class Named {
public:
    Named(std::string&& name): _name{std::move(name)} {}

    const std::string& name() const { return _name; }

private:
    std::string _name;
};

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
    const Ref<Node> child(unsigned n) const override {
        return getters(std::make_index_sequence<sizeof...(Ns)>())[n](*this);
    }

protected:
    template <std::size_t I> auto get() {
        return ref(std::get<I>(_items));
    }

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
    static const Ref<Node> item_as_node(const Tuple& self) {
        return ref(std::get<I>(self._items));
    }

    template <std::size_t... Indices>
    auto static getters(std::index_sequence<Indices...>) {
        using Get = decltype(item_as_node<0>);
        static constexpr Get* table[] = {item_as_node<Indices>...};
        return table;
    }
};

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
    const Ref<Node> child(unsigned n) const override {
        return ref(_items[n]);
    }

private:
    std::vector<ItemT> _items; // TODO: list?
};

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

    Ref<Node> child(unsigned n) override { return as_node(_items[n]); }
    const Ref<Node> child(unsigned n) const override {
        return as_node(_items[n]);
    }

private:
    std::vector<ItemT> _items; // TODO: list?
};

} // namespace ulam::ast
