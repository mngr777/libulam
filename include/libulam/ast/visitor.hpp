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
    virtual bool visit(Ref<Node> node) { return true; }

#define NODE(str, cls)                                                         \
    virtual bool visit(Ref<cls> node) { return true; }
#include <libulam/ast/nodes.inc.hpp>
#undef NODE

    unsigned level() { return _level; }
    void set_level(unsigned level) { _level = level; }

    void inc_level() { ++_level; }
    void dec_level() {
        assert(_level > 0);
        --_level;
    }

private:
    unsigned _level = 0;
};

class RecVisitor : public Visitor {
#define NODE(str, cls)                                                         \
public:                                                                        \
    virtual bool visit(Ref<cls> node) override;                                \
                                                                               \
protected:                                                                     \
    virtual bool do_visit(Ref<cls> node) { return true; }                      \
    virtual void traverse(Ref<cls> node);
#include <libulam/ast/nodes.inc.hpp>
#undef NODE
};

} // namespace ulam::ast
