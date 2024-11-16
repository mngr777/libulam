#pragma once
#include <cassert>
#include <libulam/ast/ptr.hpp>
#include <libulam/ast/visitor.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/src_loc.hpp>
#include <libulam/str_pool.hpp>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

#define ULAM_AST_NODE                                                          \
public:                                                                        \
    virtual bool accept(Visitor& v) override { return v.visit(this); }         \
                                                                               \
private:

#define ULAM_AST_TUPLE_PROP(name, index)                                       \
    auto name() { return ref(std::get<index>(_items)); }                       \
    const auto name() const { return ref(std::get<index>(_items)); }           \
    auto replace_##name(ElementT<index>&& repl) {                              \
        return replace<index>(std::move(repl));                                \
    }

#define ULAM_AST_SIMPLE_ATTR(type, name)                                       \
public:                                                                        \
    type name() { return _attr_##name; }                                       \
    type name() const { return _attr_##name; }                                 \
    void set_##name(type value) { _attr_##name = value; }                      \
                                                                               \
private:                                                                       \
    type _attr_##name{};

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

#define ULAM_AST_LINK_ATTR(type, name)                                         \
public:                                                                        \
    Link<type>* name##_link() { return &_attr_##name; }                        \
    const Link<type>* name##_link() const { return &_attr_##name; }            \
    ulam::Ref<type> name() { return _attr_##name.get(); }                      \
    ulam::Ref<const type> name() const { return _attr_##name.get(); }          \
    void set_##name(ulam::Ref<type> value) { _attr_##name.set(value); }        \
                                                                               \
private:                                                                       \
    Link<type> _attr_##name;

namespace ulam::ast {

template <typename T> class Link {
public:
    ulam::Ref<T> get() { return _value; }
    ulam::Ref<const T> get() const { return _value; }
    void set(ulam::Ref<T> value) { _value = value; }

private:
    ulam::Ref<T> _value;
};

// TODO:
// - pass AST context to `make`
// - type constraints
// - child iterators

// Variant

template <typename... Ns> using Variant = std::variant<Ptr<Ns>...>;

template <typename N, typename... Ns> bool is(const Variant<Ns...>& v) {
    return std::holds_alternative<Ptr<N>>(v);
}

// ref to node
template <typename N, typename... Ns> auto as_ref(Variant<Ns...>& v) {
    return std::visit([](auto&& ptr) -> Ref<N> { return ref(ptr); }, v);
}
template <typename N, typename... Ns> auto as_ref(const Variant<Ns...>& v) {
    return std::visit([](auto&& ptr) -> const Ref<N> { return ref(ptr); }, v);
}

// to node ptr
template <typename N, typename... Ns> auto get(Variant<Ns...>&& v) {
    return std::visit([](auto&& ptr) -> Ptr<N> { return ptr; }, std::move(v));
}
template <typename N, typename... Ns> auto get(const Variant<Ns...>&& v) {
    return std::visit([](auto&& ptr) -> Ptr<N> { return ptr; }, std::move(v));
}

class Node;

// ref to node base
template <typename... Ns> auto as_node_ref(Variant<Ns...>& v) {
    return as_ref<Node>(v);
}
template <typename... Ns> auto as_node_ref(const Variant<Ns...>& v) {
    return as_ref<Node>(v);
}

// Node base

class Node {
public:
    virtual ~Node();

    virtual bool accept(Visitor& visitor);

    virtual unsigned child_num() const { return 0; }

    virtual Ref<Node> child(unsigned n) { return nullptr; }
    virtual const Ref<Node> child(unsigned n) const { return nullptr; }

    ULAM_AST_SIMPLE_ATTR(loc_id_t, loc_id)
};

// "Named" trait (TMP)

class Named {
public:
    Named(std::string&& name): _name{std::move(name)} {}

    const std::string& name() const { return _name; }

private:
    std::string _name;
};

// TODO: replace Named
class Named_ {
public:
    Named_(str_id_t name_id): _name_id{name_id} {}

    str_id_t name_id() const { return _name_id; }

private:
    str_id_t _name_id;
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
    const Ref<Node> child(unsigned n) const override {
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
    const Ref<Node> child(unsigned n) const override { return ref(_items[n]); }

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

    Ref<Node> child(unsigned n) override { return as_node_ref(_items[n]); }
    const Ref<Node> child(unsigned n) const override {
        return as_node_ref(_items[n]);
    }

private:
    std::vector<ItemT> _items; // TODO: list?
};

} // namespace ulam::ast
