#pragma once
#include <cassert>
#include <libulam/ast/ptr.hpp>

namespace ulam::ast {

class Node;

#define NODE(str, cls) class cls;
#include <libulam/ast/nodes.inc.hpp>
#undef NODE

class Visitor {
public:
    virtual ~Visitor() {}
    virtual void visit(Ref<Node> node) {}

#define NODE(str, cls)                                                         \
    virtual void visit(Ref<cls> node) {}
#include <libulam/ast/nodes.inc.hpp>
#undef NODE
};

class RecVisitor : public Visitor {
#define NODE(str, cls)                                                         \
public:                                                                        \
    virtual void visit(Ref<cls> node) override;                                \
                                                                               \
protected:                                                                     \
    virtual bool do_visit(Ref<cls> node) { return true; }                      \
    virtual void traverse(Ref<cls> node);
#include <libulam/ast/nodes.inc.hpp>
#undef NODE
};

} // namespace ulam::ast
