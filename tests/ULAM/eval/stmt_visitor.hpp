#pragma once
#include "./env.hpp"
#include "./helper.hpp"
#include <libulam/sema/eval/expr_visitor.hpp>
#include <libulam/sema/eval/stmt_visitor.hpp>

class EvalStmtVisitor : public EvalHelper, public ulam::sema::EvalStmtVisitor {
public:
    using Base = ulam::sema::EvalStmtVisitor;
    using CondRes = ulam::sema::CondRes;

    explicit EvalStmtVisitor(EvalEnv& env): ::EvalHelper{env}, Base{env} {}

    void visit(ulam::Ref<ulam::ast::Block> node) override;
    void visit(ulam::Ref<ulam::ast::If> node) override;
    void visit(ulam::Ref<ulam::ast::For> node) override;
    void visit(ulam::Ref<ulam::ast::While> node) override;
    void visit(ulam::Ref<ulam::ast::Return> node) override;
    void visit(ulam::Ref<ulam::ast::Break> node) override;
    void visit(ulam::Ref<ulam::ast::Continue> node) override;
    void visit(ulam::Ref<ulam::ast::ExprStmt> node) override;
    void visit(ulam::Ref<ulam::ast::EmptyStmt> node) override;

protected:
    ulam::Ref<ulam::AliasType>
    type_def(ulam::Ref<ulam::ast::TypeDef> node) override;

    ulam::Ptr<ulam::Var> make_var(
        ulam::Ref<ulam::ast::TypeName> type_name,
        ulam::Ref<ulam::ast::VarDef> node,
        bool is_const) override;

private:
    void maybe_wrap_stmt(ulam::Ref<ulam::ast::Stmt> stmt, bool wrap);
};
