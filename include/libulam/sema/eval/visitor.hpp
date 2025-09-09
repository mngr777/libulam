#pragma once
#include "libulam/sema/eval/helper.hpp"
#include <libulam/ast.hpp>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/ast/visitor.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/eval/base.hpp>
#include <libulam/sema/eval/cond_res.hpp>
#include <libulam/sema/eval/flags.hpp>
#include <libulam/sema/eval/stack.hpp>
#include <libulam/sema/expr_res.hpp>
#include <libulam/sema/resolver.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope/stack.hpp>
#include <libulam/semantic/var.hpp>

namespace ulam::sema {

class EvalWhich;

class EvalVisitor : public ast::Visitor, public EvalHelper {
public:
    explicit EvalVisitor(EvalEnv& env): EvalHelper{env} {}

    void visit(Ref<ast::TypeDef> node) override;
    void visit(Ref<ast::VarDefList> node) override;

    void visit(Ref<ast::Block> node) override;
    void visit(Ref<ast::FunDefBody>) override;
    void visit(Ref<ast::If> node) override;
    void visit(Ref<ast::For> node) override;
    void visit(Ref<ast::While> node) override;
    void visit(Ref<ast::Which> node) override;
    void visit(Ref<ast::Return> node) override;
    void visit(Ref<ast::Break> node) override;
    void visit(Ref<ast::Continue> node) override;
    void visit(Ref<ast::ExprStmt> node) override;
    void visit(Ref<ast::EmptyStmt> node) override;
    void visit(Ref<ast::UnaryOp> node) override;
    void visit(Ref<ast::BinaryOp> node) override;
    void visit(Ref<ast::FunCall> node) override;
    void visit(Ref<ast::ArrayAccess> node) override;
    void visit(Ref<ast::MemberAccess> node) override;
    void visit(Ref<ast::TypeOpExpr> node) override;
    void visit(Ref<ast::Ident> node) override;

protected:
    virtual Ref<AliasType> type_def(Ref<ast::TypeDef> node);

    virtual Ref<Var>
    var_def(Ref<ast::TypeName> type_name, Ref<ast::VarDef> node, bool is_const);

    virtual Ptr<Var> make_var(
        Ref<ast::TypeName> type_name, Ref<ast::VarDef> node, bool is_const);

    virtual ExprRes ret_res(Ref<ast::Return> node);
};

} // namespace ulam::sema
