#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/expr_res.hpp>
#include <libulam/semantic/type/prim.hpp>

namespace ulam::ast {
class BinaryOp;
}

namespace ulam {

class Program;
class Value;

class VoidType : public PrimType {
public:
    VoidType(Ref<Program> program): PrimType{program} {}
};

} // namespace ulam
