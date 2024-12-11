#include <libulam/ast/nodes/params.hpp>
#include <libulam/ast/nodes/var_decl.hpp>
#include <libulam/semantic/var.hpp>

namespace ulam {

str_id_t Var::name_id() const { return _node->name().str_id(); }

} // namespace ulam
