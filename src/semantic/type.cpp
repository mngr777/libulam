#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type.hpp>

namespace ulam {

bool Type::is_basic() const {
    return this == static_cast<Ref<const Type>>(basic());
}

_TypeDec::~_TypeDec() {}

CompType::~CompType() {}

} // namespace ulam
