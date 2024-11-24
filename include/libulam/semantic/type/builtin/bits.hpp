#include <libulam/ast/ptr.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/expr_res.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/type/prim.hpp>

namespace ulam::ast {
class BinaryOp;
}

namespace ulam {

class Program;
class Value;

class BitsType : public _PrimType<BitsId, 1, 4096, 8> {
public:
    BitsType(Ref<Program> program, bitsize_t bitsize):
        _PrimType{program, bitsize} {}
};

using BitsTypeTpl = _PrimTypeTpl<BitsType>;

} // namespace ulam
