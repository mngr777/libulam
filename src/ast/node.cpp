#include <libulam/ast/node.hpp>

namespace ulam::ast {

Node::~Node() {}

bool Node::accept(Visitor& v) { return v.visit(this); }

} // namespace ulam::ast
