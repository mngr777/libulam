#pragma once
#include <cassert>
#include <libulam/ast/node.hpp>
#include <libulam/lang/op.hpp>
#include <libulam/types.hpp>

namespace ulam::ast {

class Expr : public Node {};

class Name : public Expr {
    ULAM_AST_NODE
};

class Ident : public Expr {};

class TypeIdent : public Ident {};

class FunCall : public Expr {};

class ParenExpr : public Expr {
    ULAM_AST_NODE
public:
    ParenExpr(Ptr<Expr>&& inner): _inner{std::move(inner)} {}

    Expr* inner() { return _inner.get(); }

private:
    Ptr<Expr> _inner;
};

class OpExpr : public Expr {
public:
    OpExpr(Op op): _op{op} {}

    Op op() const { return _op; }

protected:
    Op _op;
};

class BinaryOp : public OpExpr {
    ULAM_AST_NODE
public:
    BinaryOp(Op op, Ptr<Expr>&& lhs, Ptr<Expr>&& rhs):
        OpExpr{op}, _lhs{std::move(lhs)}, _rhs{std::move(rhs)} {}

    Expr* lhs() { return _lhs.get(); }
    Expr* rhs() { return _rhs.get(); }

    unsigned child_num() const override { return 1; }

    Node* child(unsigned n) override {
        assert(n < child_num());
        return n == 0 ? lhs() : rhs();
    }

private:
    Ptr<Expr> _lhs;
    Ptr<Expr> _rhs;
};

class UnaryOp : public OpExpr {
public:
    UnaryOp(Op op, Ptr<Expr>&& arg): OpExpr{op}, _arg{std::move(arg)} {
        assert(_arg);
    }

    Expr* arg() { return _arg.get(); }

    unsigned child_num() const override { return 1; }

    Node* child(unsigned n) override {
        assert(n < child_num());
        return arg();
    }

private:
    Ptr<Expr> _arg;
};

class VarRef : public Expr {};

class Cast : public Expr {};

class BoolLit : public Expr {};

class Number : public Expr {
    ULAM_AST_NODE
};

class String : public Expr {
    ULAM_AST_NODE
};

} // namespace ulam::ast
