#pragma once
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/ast/nodes/params.hpp>
#include <libulam/ast/nodes/stmt.hpp>

namespace ulam::ast {

class FunCall : public Expr {
    ULAM_AST_NODE
public:
    FunCall(Ptr<Expr>&& obj, Ptr<ArgList>&& args):
        _obj{std::move(obj)}, _args{std::move(args)} {}

    Expr* obj() { return _obj.get(); }
    ArgList* args() { return _args.get(); }

private:
    Ptr<Expr> _obj;
    Ptr<ArgList> _args;
};

class ArrayAccess : public Expr {
    ULAM_AST_NODE
public:
    ArrayAccess(Ptr<Expr>&& array, Ptr<Expr>&& index):
        _array{std::move(array)}, _index{std::move(index)} {}

    Expr* array() { return _array.get(); }
    Expr* index() { return _index.get(); }

private:
    Ptr<Expr> _array;
    Ptr<Expr> _index;
};


}
