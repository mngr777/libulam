#pragma once
#include <libulam/assert.hpp>
#include <libulam/memory/ptr.hpp>

namespace ulam::ast {

class Node;

#define NODE(str, cls) class cls;
#include <libulam/ast/nodes.inc.hpp>

class Visitor {
public:
    virtual ~Visitor() {}
    virtual void visit(Ref<Node> node) {}

#define NODE(str, cls)                                                         \
    virtual void visit(Ref<cls> node) {}
#include <libulam/ast/nodes.inc.hpp>
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
};

} // namespace ulam::ast
