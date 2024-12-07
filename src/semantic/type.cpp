#include <libulam/ast/nodes/module.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type.hpp>

namespace ulam {

str_id_t AliasType::name_id() const {
    return _node->alias_id();
}

} // namespace ulam
