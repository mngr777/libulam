#pragma once
#include <libulam/ast/node.hpp>

namespace ulam::ast {

class Stmt : public Node {};

class Block : public Stmt {
    ULAM_AST_NODE
public:
    void add(Ptr<Stmt>&& stmt) {
        _body.push_back(std::move(stmt));
    }

    unsigned child_num() const override { return _body.size(); }

    Node* child(unsigned n) override {
        assert(n < child_num());
        return _body[n].get();
    }

private:
    List<Stmt> _body;
};

} // namespace ulam::ast
