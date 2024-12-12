#include <cassert>
#include <libulam/ast/nodes/params.hpp>
#include <libulam/ast/nodes/var_decl.hpp>
#include <libulam/semantic/var.hpp>

namespace ulam {

str_id_t Var::name_id() const { return _node->name().str_id(); }

void Var::set_type(Ref<Type> type) {
    assert(!_type);
    _type = type;
}

} // namespace ulam
