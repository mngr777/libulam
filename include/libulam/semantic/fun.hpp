#pragma once
#include <libulam/ast/ptr.hpp>
#include <libulam/memory/ptr.hpp>

namespace ulam::ast {
class FunDef;
class ParamList;
} // namespace ulam::ast

namespace ulam {

class Type;

class Fun {
public:
    Fun(ast::Ref<ast::FunDef> def, Ref<Type> ret_type) {}

    Ref<Type> ret_type() { return _ret_type; }

    void add_param(); // ?

private:
    Ref<Type> _ret_type;
};

class FunTpl {
public:
    FunTpl(ast::Ref<ast::FunDef> def): _def{def} {}

    auto def() { return _def; }

private:
    ast::Ref<ast::FunDef> _def;
};

} // namespace ulam
