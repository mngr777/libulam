#pragma once
#include <libulam/ast/ptr.hpp>
#include <libulam/memory/ptr.hpp>
#include <list>

namespace ulam::ast {
class FunDef;
class FunDefBody;
class ParamList;
class TypeName;
} // namespace ulam::ast

namespace ulam {

class Type;

class FunOverload {
public:
    FunOverload(ast::Ref<ast::FunDef> node) {}

    ast::Ref<ast::FunDef> node() { return _node; }
    ast::Ref<ast::TypeName> ret_type_node();
    ast::Ref<ast::ParamList> params_node();
    ast::Ref<ast::FunDefBody> body_node();

private:
    ast::Ref<ast::FunDef> _node;
    Ref<Type> _ret_type;
};

class Fun {
public:
    Fun() {}

    void add_overload(ast::Ref<ast::FunDef> node) {
        _overloads.push_back(ulam::make<FunOverload>(node));
    }

private:
    std::list<Ptr<FunOverload>> _overloads;
};

} // namespace ulam
