#pragma once
#include "./stringifier.hpp"
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/eval/expr_visitor.hpp>
#include <libulam/sema/eval/init.hpp>
#include <libulam/sema/eval/visitor.hpp>
#include <libulam/sema/expr_res.hpp>
#include <libulam/semantic/program.hpp>
#include <string>

class EvalVisitor : public ulam::sema::EvalVisitor {
public:
    EvalVisitor(ulam::Ref<ulam::Program> program):
        ulam::sema::EvalVisitor{program},
        _stringifier{program->builtins(), program->text_pool()} {}

    void visit(ulam::Ref<ulam::ast::Block> node) override;
    void visit(ulam::Ref<ulam::ast::Return> node) override;
    void visit(ulam::Ref<ulam::ast::ExprStmt> node) override;

    bool in_main() { return _stack.size() == 1; }

    const std::string& data() const { return _data; }

    ulam::Ptr<ulam::sema::EvalExprVisitor>
    expr_visitor(ulam::Ref<ulam::Scope> scope) override;

    ulam::Ptr<ulam::sema::EvalInit>
    init_helper(ulam::Ref<ulam::Scope> scope) override;

    ulam::Ptr<ulam::sema::EvalCast>
    cast_helper(ulam::Ref<ulam::Scope> scope) override;

    ulam::Ptr<ulam::sema::EvalFuncall>
    funcall_helper(ulam::Ref<ulam::Scope> scope) override;

protected:
    ulam::Ref<ulam::AliasType>
    type_def(ulam::Ref<ulam::ast::TypeDef> node) override;

    ulam::Ref<ulam::Var> var_def(
        ulam::Ref<ulam::ast::TypeName> type_name,
        ulam::Ref<ulam::ast::VarDef> node) override;

    ulam::sema::ExprRes eval_expr(ulam::Ref<ulam::ast::Expr> expr) override;

private:
    void append(std::string data);

    Stringifier _stringifier;
    std::string _data;
};
