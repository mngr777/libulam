#include <libulam/ast/nodes.hpp>
#include <libulam/ast/visitor.hpp>

namespace ulam::ast {

#define NODE(str, cls)                                                         \
    void RecVisitor::visit(Ref<cls> node) {                                    \
        if (do_visit(node))                                                    \
            traverse(node);                                                    \
    }                                                                          \
    void RecVisitor::traverse(Ref<cls> node) {                                 \
        for (unsigned n = 0; n < node->child_num(); ++n) {                     \
            auto child = node->child(n);                                       \
            if (child)                                                         \
                child->accept(*this);                                          \
        }                                                                      \
    }
#include <libulam/ast/nodes.inc.hpp>
#undef NODE

} // namespace ulam::ast
