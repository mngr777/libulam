#include <libulam/ast/node.hpp>

namespace ulam::ast {

class Visitor;

void traverse(Node* node, Visitor& v);

} // namespace ulam::ast
