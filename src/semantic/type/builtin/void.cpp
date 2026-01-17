#include <libulam/assert.hpp>
#include <libulam/semantic/type/builtin/void.hpp>

namespace ulam {

Ref<ArrayType> VoidType::array_type(array_size_t size) { unreachable(); }

Ref<RefType> VoidType::ref_type() { unreachable(); }

} // namespace ulam
