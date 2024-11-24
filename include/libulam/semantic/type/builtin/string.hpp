#include <libulam/ast/ptr.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/expr_res.hpp>
#include <libulam/semantic/type/prim.hpp>

namespace ulam::ast {
class BinaryOp;
}

namespace ulam {

class Program;

class StringType : public PrimType {
public:
    StringType(Ref<Program> program): PrimType{program} {}

    ExprRes binary_op(
        ast::Ref<ast::BinaryOp> node,
        Value& lhs,
        Ref<Type> rhs_type,
        Value& rhs) override {
        return {ExprError::NoOperator};
    }
};

} // namespace ulam
