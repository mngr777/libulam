#include "libulam/ast/node.hpp"
#include "libulam/ast/traversal.hpp"

namespace ulam::ast {

Node::~Node() {}

bool Node::accept(Visitor& v) { return v.visit(*this); }
bool Node::accept(Visitor& v) const { return v.visit(*this); }

} // namespace ulam::ast
