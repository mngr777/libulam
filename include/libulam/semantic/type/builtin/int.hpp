#include <libulam/ast/ptr.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/type/prim.hpp>
#include <libulam/semantic/value.hpp>

namespace ulam {

class Program;
class Value;

class IntType : public _PrimType<IntId, 2, ULAM_MAX_INT_SIZE, 32> {
public:
    IntType(Ref<Program> program, bitsize_t bitsize):
        _PrimType{program, bitsize} {}

    bool is_convertible(Ref<const Type> type) override;

    // Value cast(
    //     ast::Ref<ast::Node> node,
    //     Ref<const Type> type,
    //     const Value& value,
    //     bool is_impl = true) override;

    // ExprRes binary_op(
    //     ast::Ref<ast::BinaryOp> node,
    //     Value& left,
    //     Ref<const Type> right_type,
    //     const Value& right) override;

private:
    // ExprRes binary_op_int(
    //     ast::Ref<ast::BinaryOp> node,
    //     Value& left,
    //     Ref<const Type> right_type,
    //     const Value& right);
};

using IntTypeTpl = _PrimTypeTpl<IntType>;

} // namespace ulam
