#pragma once
#include "libulam/ast.hpp"
#include "libulam/ast/nodes/module.hpp"
#include "libulam/ast/nodes/params.hpp"
#include "libulam/lang/ops.hpp"
#include "libulam/token.hpp"
#include "libulam/preproc.hpp"
#include <string>

namespace ulam {

class Preproc;
class SrcMngr;

class Parser {
public:
    Parser(SrcMngr& sm): _sm{sm}, _pp{sm} {}

    ast::Ptr<ast::Module> parse_file(const std::filesystem::path& path);
    ast::Ptr<ast::Module> parse_str(const std::string& text);

private:
    void consume();
    void expect(tok::Type type);
    bool eof();

    void diag(std::string message) {}

    ast::Ptr<ast::Module> parse_module();
    ast::Ptr<ast::ClassDef> parse_class_def();
    ast::Ptr<ast::TypeDef> parse_type_def();
    ast::Ptr<ast::VarDef> parse_var_def();
    ast::Ptr<ast::VarDef>
    parse_var_def_rest(ast::Ptr<ast::Expr>&& type, std::string&& name_);
    ast::Ptr<ast::FunDef>
    parse_fun_def_rest(ast::Ptr<ast::Expr>&& ret_type, std::string&& name_);
    ast::Ptr<ast::ParamList> parse_param_list();
    ast::Ptr<ast::Param> parse_param();
    ast::Ptr<ast::Block> parse_block();

    ast::Ptr<ast::Expr> parse_expr();
    ast::Ptr<ast::Expr> parse_expr_climb(ops::Prec min_prec);
    ast::Ptr<ast::Expr> parse_cast_expr();
    ast::Ptr<ast::Expr> parse_paren_expr_or_cast();
    ast::Ptr<ast::FunCall> parse_funcall(ast::Ptr<ast::Expr>&& obj);
    ast::Ptr<ast::ArrayAccess> parse_array_access(ast::Ptr<ast::Expr>&& array);
    ast::Ptr<ast::TypeName> parse_type_name();
    ast::Ptr<ast::Name> parse_name();
    ast::Ptr<ast::Number> parse_number();
    ast::Ptr<ast::String> parse_string();

    template <typename N, typename... Args> ast::Ptr<N> tree(Args&&... args) {
        return ast::make<N>(std::forward<Args>(args)...);
    }

    ast::Ptr<ast::Expr>
    binop_tree(Op op, ast::Ptr<ast::Expr>&& lhs, ast::Ptr<ast::Expr>&& rhs);

    std::string tok_str();

    SrcMngr& _sm;
    Preproc _pp;

    Token _tok;
};

} // namespace ulam
