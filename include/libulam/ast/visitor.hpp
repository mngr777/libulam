#pragma once
#include <cassert>

namespace ulam::ast {

class Node;
#define NODE(str, cls) class cls;
#include <libulam/ast/nodes.inc.hpp>
#undef NODE

class Visitor {
public:
    virtual ~Visitor() {}
    virtual bool visit(Node& node) { return true; }

#define NODE(str, cls)                                                         \
    virtual bool visit(cls& node) { return true; }
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

} // namespace ulam::ast
