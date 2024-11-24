#include <libulam/ast/ptr.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/expr_res.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/type/prim.hpp>
#include <libulam/semantic/value.hpp>

namespace ulam::ast {
class BinaryOp;
}

namespace ulam {

class Program;
class Value;

class UnsignedType : public _PrimType<UnsignedId, 1, ULAM_MAX_INT_SIZE, 32> {
public:
    UnsignedType(Ref<Program> program, bitsize_t bitsize):
        _PrimType{program, bitsize} {}
};

using UnsignedTypeTpl = _PrimTypeTpl<UnsignedType>;

} // namespace ulam
