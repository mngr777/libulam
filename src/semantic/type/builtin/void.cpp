#include "src/utils/unreachable.hpp"
#include <libulam/semantic/type/builtin/void.hpp>

namespace ulam {

Ref<ArrayType> VoidType::array_type(array_size_t size) { utils::unreachable(); }

Ref<RefType> VoidType::ref_type() { utils::unreachable(); }

} // namespace ulam
