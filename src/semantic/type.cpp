#include <libulam/ast/nodes/module.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type.hpp>

namespace ulam {

bool Type::is_basic() const {
    return this == static_cast<Ref<const Type>>(basic());
}

str_id_t AliasType::name_id() const { return _node->name().str_id(); }

_TypeDec::~_TypeDec() {}

CompType::~CompType() {}

} // namespace ulam
