#pragma once
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

class IntType : public _PrimType<IntId, 2, 64, 32> {
public:
    IntType(Ref<Program> program, bitsize_t bitsize):
        _PrimType{program, bitsize} {}

    ExprRes binary_op(
        ast::Ref<ast::BinaryOp> node,
        Value& lhs,
        Ref<Type> rhs_type,
        Value& rhs) override {
        return {ExprError::NoOperator};
    }
};

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

class BoolType : public _PrimType<BoolId, 1, 64, 1> {
public:
    BoolType(Ref<Program> program, bitsize_t bitsize):
        _PrimType{program, bitsize} {}

    ExprRes binary_op(
        ast::Ref<ast::BinaryOp> node,
        Value& lhs,
        Ref<Type> rhs_type,
        Value& rhs) override {
        return {ExprError::NoOperator};
    }
};

class UnaryType : public _PrimType<UnaryId, 1, 64, 32> {
public:
    UnaryType(Ref<Program> program, bitsize_t bitsize):
        _PrimType{program, bitsize} {}

    ExprRes binary_op(
        ast::Ref<ast::BinaryOp> node,
        Value& lhs,
        Ref<Type> rhs_type,
        Value& rhs) override {
        return {ExprError::NoOperator};
    }
};

class BitsType : public _PrimType<BitsId, 1, 4096, 8> {
public:
    BitsType(Ref<Program> program, bitsize_t bitsize):
        _PrimType{program, bitsize} {}

    ExprRes binary_op(
        ast::Ref<ast::BinaryOp> node,
        Value& lhs,
        Ref<Type> rhs_type,
        Value& rhs) override {
        return {ExprError::NoOperator};
    }
};

class VoidType : public PrimType {
public:
    VoidType(Ref<Program> program): PrimType{program} {}
};

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

using IntTypeTpl = _PrimTypeTpl<IntType>;
using UnsignedTypeTpl = _PrimTypeTpl<UnsignedType>;
using BoolTypeTpl = _PrimTypeTpl<BoolType>;
using UnaryTypeTpl = _PrimTypeTpl<UnaryType>;
using BitsTypeTpl = _PrimTypeTpl<BitsType>;

} // namespace ulam
