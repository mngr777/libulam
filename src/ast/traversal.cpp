#include <cassert>
#include <libulam/ast/nodes.hpp>
#include <libulam/ast/traversal.hpp>
#include <libulam/ast/visitor.hpp>

namespace ulam::ast {

namespace {

void _traverse(Ref<Node> node, Visitor& v) {
    if (!node || !node->accept(v))
        return;

    v.inc_level();
    for (std::size_t n = 0; n < node->child_num(); ++n)
        _traverse(node->child(n), v);
    v.dec_level();
}

} // namespace

void traverse(Ref<Node> node, Visitor& v) {
    v.set_level(0);
    _traverse(node, v);
    assert(v.level() == 0);
}

} // namespace ulam::ast
