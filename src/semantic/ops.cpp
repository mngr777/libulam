#include <cassert>
#include <libulam/semantic/ops.hpp>
#include <map>

namespace ulam::ops {
namespace {

auto make_fun_op_table() {
    std::map<std::string_view, Op> table;
#define FUN_OP(name, op) table[name] = Op::op;
#include <libulam/semantic/fun_ops.inc.hpp>
#undef FUN_OP
    return table;
}

} // namespace

const char* str(Op op) {
#define OP(str, op)                                                            \
    case Op::op:                                                               \
        return str;
    switch (op) {
#include <libulam/semantic/ops.inc.hpp>
    default:
        assert(false);
    }
#undef OP
}

Op fun_name_op(const std::string_view name) {
    static auto table{make_fun_op_table()};
    auto it = table.find(name);
    return (it != table.end()) ? it->second : Op::None;
}

bool is_overloadable(Op op) {
    switch (op) {
    case Op::Assign:
    case Op::ArrayAccess:
    case Op::Equal:
    case Op::NotEqual:
    case Op::Prod:
    case Op::Quot:
    case Op::Rem:
    case Op::Sum:
    case Op::Diff:
    case Op::Less:
    case Op::LessOrEq:
    case Op::Greater:
    case Op::GreaterOrEq:
    case Op::PreInc:
    case Op::PreDec:
    case Op::PostInc:
    case Op::PostDec:
    case Op::AssignProd:
    case Op::AssignQuot:
    case Op::AssignRem:
    case Op::AssignSum:
    case Op::AssignDiff:
    case Op::Negate:
    case Op::And:
    case Op::Or:
    case Op::ShiftLeft:
    case Op::ShiftRight:
    case Op::BwNot:
    case Op::BwAnd:
    case Op::BwOr:
    case Op::BwXor:
    case Op::AssignShiftLeft:
    case Op::AssignShiftRight:
    case Op::AssignBwAnd:
    case Op::AssignBwOr:
    case Op::AssignBwXor:
    case Op::UnaryPlus:
    case Op::UnaryMinus:
        return true;
    default:
        return false;
    }
}

Kind kind(Op op) {
    switch (op) {
    case Op::Assign:
        return Kind::Assign;
    case Op::Equal:
    case Op::NotEqual:
        return Kind::Equality;
    case Op::Less:
    case Op::LessOrEq:
    case Op::Greater:
    case Op::GreaterOrEq:
        return Kind::Comparison;
    case Op::Prod:
    case Op::Quot:
    case Op::Rem:
    case Op::Sum:
    case Op::Diff:
    case Op::UnaryPlus:
    case Op::UnaryMinus:
    case Op::PreInc:
    case Op::PreDec:
    case Op::PostInc:
    case Op::PostDec:
    case Op::AssignProd:
    case Op::AssignQuot:
    case Op::AssignRem:
    case Op::AssignSum:
    case Op::AssignDiff:
        return Kind::Numeric;
    case Op::Negate:
    case Op::And:
    case Op::Or:
        return Kind::Logical;
    case Op::ShiftLeft:
    case Op::ShiftRight:
    case Op::BwNot:
    case Op::BwAnd:
    case Op::BwXor:
    case Op::BwOr:
    case Op::AssignShiftLeft:
    case Op::AssignShiftRight:
    case Op::AssignBwAnd:
    case Op::AssignBwXor:
    case Op::AssignBwOr:
        return Kind::Bitwise;
    case Op::Is:
    case Op::As:
        return Kind::Objective;
    default:
        assert(false);
    }
}

Op negation(Op op) {
    switch (op) {
    case Op::Equal:
        return Op::NotEqual;
    case Op::NotEqual:
        return Op::Equal;
    case Op::Less:
        return Op::GreaterOrEq;
    case Op::GreaterOrEq:
        return Op::Less;
    case Op::Greater:
        return Op::LessOrEq;
    case Op::LessOrEq:
        return Op::Greater;
    default:
        return Op::None;
    }
}

bool is_numeric(Op op) { return kind(op) == Kind::Numeric; }

bool is_logical(Op op) { return kind(op) == Kind::Logical; }

bool is_bitwise(Op op) { return kind(op) == Kind::Bitwise; }

bool is_assign(Op op) {
    switch (op) {
    case Op::Assign:
    case Op::AssignProd:
    case Op::AssignQuot:
    case Op::AssignRem:
    case Op::AssignSum:
    case Op::AssignDiff:
    case Op::AssignShiftLeft:
    case Op::AssignShiftRight:
    case Op::AssignBwAnd:
    case Op::AssignBwXor:
    case Op::AssignBwOr:
        return true;
    default:
        return false;
    }
}

Op non_assign(Op op) {
    switch (op) {
    case Op::Assign:
        return Op::None;
    case Op::AssignProd:
        return Op::Prod;
    case Op::AssignQuot:
        return Op::Quot;
    case Op::AssignRem:
        return Op::Rem;
    case Op::AssignSum:
        return Op::Sum;
    case Op::AssignDiff:
        return Op::Diff;
    case Op::AssignShiftLeft:
        return Op::ShiftLeft;
    case Op::AssignShiftRight:
        return Op::ShiftRight;
    case Op::AssignBwAnd:
        return Op::BwAnd;
    case Op::AssignBwXor:
        return Op::BwXor;
    case Op::AssignBwOr:
        return Op::BwOr;
    default:
        assert(!is_assign(op));
        return op;
    }
}

bool is_unary_op(Op op) { return is_unary_pre_op(op) || is_unary_post_op(op); }

bool is_unary_pre_op(Op op) {
    switch (op) {
    case Op::UnaryPlus:
    case Op::UnaryMinus:
    case Op::PreInc:
    case Op::PreDec:
    case Op::Negate:
    case Op::BwNot:
        return true;
    default:
        return false;
    }
}

bool is_unary_post_op(Op op) {
    switch (op) {
    case Op::PostInc:
    case Op::PostDec:
    case Op::Is:
    case Op::As:
        return true;
    default:
        return false;
    }
}

bool is_inc_dec(Op op) {
    switch (op) {
    case Op::PreInc:
    case Op::PreDec:
    case Op::PostInc:
    case Op::PostDec:
        return true;
    default:
        return false;
    }
}

Prec prec(Op op) {
    switch (op) {
    case Op::Is:
    case Op::As:
    case Op::FunCall:
    case Op::ArrayAccess:
    case Op::MemberAccess:
    case Op::PreInc:
    case Op::PreDec:
    case Op::Negate:
    case Op::BwNot:
        return 14;
    case Op::Cast:
    case Op::UnaryMinus:
    case Op::UnaryPlus:
    case Op::PostInc:
    case Op::PostDec:
        return 13;
    case Op::Prod:
    case Op::Quot:
    case Op::Rem:
        return 12;
    case Op::Sum:
    case Op::Diff:
        return 11;
    case Op::ShiftLeft:
    case Op::ShiftRight:
        return 10;
    case Op::Less:
    case Op::LessOrEq:
    case Op::Greater:
    case Op::GreaterOrEq:
        return 9;
    case Op::Equal:
    case Op::NotEqual:
        return 8;
    case Op::BwAnd:
        return 7;
    case Op::BwXor:
        return 6;
    case Op::BwOr:
        return 5;
    case Op::And:
        return 4;
    case Op::Or:
        return 3;
    case Op::Ternary:
        return 2;
    case Op::Assign:
    case Op::AssignSum:
    case Op::AssignDiff:
    case Op::AssignProd:
    case Op::AssignQuot:
    case Op::AssignRem:
    case Op::AssignShiftLeft:
    case Op::AssignShiftRight:
    case Op::AssignBwAnd:
    case Op::AssignBwXor:
    case Op::AssignBwOr:
        return 1;
    case Op::Comma:
        return 0;
    default:
        return -1;
    }
}

Prec right_prec(Op op) { return prec(op) + (assoc(op) == Assoc::Left ? 1 : 0); }

Assoc assoc(Op op) {
    switch (op) {
    case Op::Is:
    case Op::As:
    case Op::MemberAccess:
    case Op::Prod:
    case Op::Quot:
    case Op::Rem:
    case Op::Sum:
    case Op::Diff:
    case Op::ShiftLeft:
    case Op::ShiftRight:
    case Op::Less:
    case Op::LessOrEq:
    case Op::Greater:
    case Op::GreaterOrEq:
    case Op::Equal:
    case Op::NotEqual:
    case Op::BwAnd:
    case Op::BwXor:
    case Op::BwOr:
    case Op::And:
    case Op::Or:
    case Op::PostInc:
    case Op::PostDec:
    case Op::Ternary:
    case Op::Comma:
        return Assoc::Left;
    case Op::UnaryMinus:
    case Op::UnaryPlus:
    case Op::PreInc:
    case Op::PreDec:
    case Op::Assign:
    case Op::AssignSum:
    case Op::AssignDiff:
    case Op::AssignProd:
    case Op::AssignQuot:
    case Op::AssignRem:
    case Op::AssignShiftLeft:
    case Op::AssignShiftRight:
    case Op::AssignBwAnd:
    case Op::AssignBwXor:
    case Op::AssignBwOr:
        return Assoc::Right;
    default:
        assert(false);
    }
}

} // namespace ulam::ops
