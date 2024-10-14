#pragma once
#include <cassert>
#include <libulam/ast/visitor.hpp>
#include <memory>
#include <utility>
#include <variant>
#include <vector>

#define ULAM_AST_NODE                                                          \
public:                                                                        \
    virtual bool accept(Visitor& v) override { return v.visit(*this); }        \
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

// TODO: use list with child iterator
template <typename N> using List = std::vector<Ptr<N>>;
template <typename... Ns> using ListOf = std::vector<Variant<Ns...>>;

class Node {
public:
    virtual ~Node();

    virtual bool accept(Visitor& visitor);

    // TODO: child iterator

    virtual unsigned child_num() const { return 0; }

    virtual Node* child(unsigned n) {
        assert(n < child_num()); // TODO: make virtual protected `get_child` function
        return nullptr;
    }
};

} // namespace ulam::ast
