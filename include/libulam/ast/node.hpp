#pragma once
#include <cassert>
#include <libulam/ast/visitor.hpp>
#include <memory>
#include <utility>

namespace ulam::ast {

class Node;

// TODO: type constraints
template <typename N> using Ptr = std::unique_ptr<N>;

// TODO: pass context
template <typename N, typename... Args> Ptr<N> make(Args&&... args) {
    return std::make_unique<N>(std::forward<Args>(args)...);
}

#define ULAM_AST_NODE                                                          \
public:                                                                        \
    virtual bool accept(Visitor& v) override { return v.visit(*this); }        \
                                                                               \
private:

class Node {
public:
    virtual ~Node();

    virtual bool accept(Visitor& visitor);

    virtual unsigned child_num() const { return 0; }

    virtual Node* child(unsigned n) {
        assert(n < child_num());
        return nullptr;
    }
};

} // namespace ulam::ast
