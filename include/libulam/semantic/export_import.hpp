#pragma once
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/ast/nodes/module.hpp>
#include <list>

namespace ulam {

class Export {
public:
    explicit Export(ast::Ref<ast::ClassDef> node): _node{node} {}

    auto node() { return _node; }

private:
    ast::Ref<ast::ClassDef> _node;
};

class Import {
public:
    auto& nodes() { return _nodes; }
    const auto& nodes() const { return _nodes; }

    void add_node(ast::Ref<ast::TypeSpec> node) { _nodes.push_back(node); }

private:
    std::list<ast::Ref<ast::TypeSpec>> _nodes;
};

} // namespace ulam
