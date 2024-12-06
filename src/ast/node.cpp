#include <libulam/ast/node.hpp>

namespace ulam::ast {

Node::~Node() {}

void Node::accept(Visitor& v) { v.visit(this); }

} // namespace ulam::ast
