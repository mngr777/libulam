#include <libulam/ast/node.hpp>

namespace ulam::ast {

class Visitor;

void traverse(Ref<Node> node, Visitor& v);

} // namespace ulam::ast
