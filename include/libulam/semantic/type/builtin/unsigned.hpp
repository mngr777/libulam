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

class UnsignedType : public _PrimType<UnsignedId, 1, 64, 32> {
public:
    UnsignedType(Ref<Program> program, bitsize_t bitsize):
        _PrimType{program, bitsize} {}

    ExprRes binary_op(
        ast::Ref<ast::BinaryOp> node,
        Value& lhs,
        Ref<Type> rhs_type,
        Value& rhs) override {
        return {ExprError::NoOperator};
    }
};

using UnsignedTypeTpl = _PrimTypeTpl<UnsignedType>;

} // namespace ulam
