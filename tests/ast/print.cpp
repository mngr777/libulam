#include "./print.hpp"
#include "libulam/ast/nodes/expr.hpp"
#include <cassert>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/ast/visitor.hpp>
#include <libulam/semantic/module.hpp>
#include <libulam/semantic/ops.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/type_ops.hpp>

namespace test::ast {

// PrinterBase

PrinterBase::PrinterBase(std::ostream& os, ulam::Ref<ulam::ast::Root> ast):
    _os{os}, _ast{ast} {}

PrinterBase::~PrinterBase() { assert(_lvl == 0); }

void PrinterBase::print() { visit(_ast); }

void PrinterBase::visit(ulam::Ref<ulam::ast::ModuleDef> node) {
    if (do_visit(node)) {
        inc_lvl();
        traverse(node);
        dec_lvl();
    }
}

void PrinterBase::traverse(ulam::Ref<ulam::ast::ModuleDef> node) {
    traverse_with_indent(node);
}

void PrinterBase::visit(ulam::Ref<ulam::ast::ClassDefBody> node) {
    if (do_visit(node)) {
        inc_lvl();
        traverse(node);
        dec_lvl();
    }
}

void PrinterBase::traverse(ulam::Ref<ulam::ast::ClassDefBody> node) {
    traverse_with_indent(node);
}

void PrinterBase::visit(ulam::Ref<ulam::ast::FunRetType> node) {
    visit(node->type_name());
    if (node->has_array_dims())
        print_array_dims(node->array_dims());
    if (node->is_ref())
        _os << "&";
}

void PrinterBase::visit(ulam::Ref<ulam::ast::FunDefBody> node) {
    if (do_visit(node)) {
        inc_lvl();
        traverse(node);
        dec_lvl();
    }
}

void PrinterBase::traverse(ulam::Ref<ulam::ast::FunDefBody> node) {
    traverse_with_indent(node);
}

void PrinterBase::visit(ulam::Ref<ulam::ast::Block> node) {
    if (do_visit(node)) {
        inc_lvl();
        traverse(node);
        dec_lvl();
    }
}

void PrinterBase::traverse(ulam::Ref<ulam::ast::Block> node) {
    traverse_with_indent(node);
}

void PrinterBase::print_var_decl(ulam::Ref<ulam::ast::VarDecl> node) {
    // &
    if (node->is_ref())
        _os << "&";
    // name
    _os << name(node);
    // []
    if (node->has_array_dims())
        print_array_dims(node->array_dims());
    // = value
    if (node->has_init()) {
        auto init = node->init();
        if (!init->is_constr_call())
            _os << " = ";
        accept_me(init);
    }
}

void PrinterBase::print_array_dims(ulam::Ref<ulam::ast::ExprList> exprs) {
    assert(exprs->child_num() > 0);
    for (unsigned n = 0; n < exprs->child_num(); ++n) {
        _os << "[";
        auto expr = exprs->get(n);
        if (expr)
            accept_me(expr);
        _os << "]";
    }
}

void PrinterBase::traverse_with_indent(ulam::Ref<ulam::ast::Node> node) {
    for (unsigned n = 0; n < node->child_num(); ++n) {
        indent();
        accept_me(node->child(n));
        _os << nl();
    }
}

void PrinterBase::accept_me(ulam::Ref<ulam::ast::Node> node) {
    if (node) {
        node->accept(*this);
    } else {
        on_empty();
    }
}

void PrinterBase::on_empty() { _os << "<empty>"; }

std::ostream& PrinterBase::indent() {
    for (unsigned i = 0; i < _lvl * options.ident; ++i)
        _os << " ";
    return _os;
}

const char* PrinterBase::paren_l() { return options.expl_parens ? "(" : ""; }

const char* PrinterBase::paren_r() { return options.expl_parens ? "(" : ""; }

const char* PrinterBase::nl() { return _no_newline ? "" : "\n"; }

const std::string_view PrinterBase::name(ulam::Ref<ulam::ast::Named> node) {
    return str(node->name().str_id());
}

const std::string_view PrinterBase::str(ulam::str_id_t str_id) {
    return ast()->ctx().str(str_id);
}

ulam::Ref<ulam::ast::Root> PrinterBase::ast() {
    assert(_ast);
    return _ast;
}

void PrinterBase::inc_lvl() { ++_lvl; }

void PrinterBase::dec_lvl() {
    assert(_lvl > 0);
    --_lvl;
}

bool PrinterBase::set_no_indent(bool val) {
    bool old = _no_indent;
    _no_indent = val;
    return old;
}

bool PrinterBase::set_no_newline(bool val) {
    bool old = _no_newline;
    _no_newline = val;
    return old;
}

// Printer

void Printer::visit(ulam::Ref<ulam::ast::ClassDef> node) {
    // name
    _os << class_kind_str(node->kind()) << " " << name(node);
    // params
    if (node->has_params()) {
        assert(node->params()->child_num() > 0);
        _os << " ";
        visit(node->params());
    }
    // ancestors
    if (node->has_ancestors()) {
        assert(node->ancestors()->child_num() > 0);
        _os << " : ";
        traverse_list(node->ancestors(), " + ");
    }
    // body
    assert(node->has_body());
    _os << " {" << nl();
    visit(node->body());
    indent() << "}" << nl();
}

void Printer::visit(ulam::Ref<ulam::ast::VarDef> node) { print_var_decl(node); }

void Printer::visit(ulam::Ref<ulam::ast::FunDef> node) {
    assert(node->has_ret_type() || node->is_constructor());
    // ret type, name
    if (node->is_constructor()) {
        _os << name(node);
    } else {
        visit(node->ret_type());
        _os << " " << name(node);
    }
    // params
    assert(node->has_params());
    visit(node->params());
    // body
    if (node->has_body()) {
        _os << " {" << nl();
        visit(node->body());
        indent() << "}";
    } else {
        _os << " native;";
    }
}

void Printer::visit(ulam::Ref<ulam::ast::TypeSpec> node) {
    if (node->is_builtin()) {
        _os << ulam::builtin_type_str(node->builtin_type_id());
    } else {
        accept_me(node->ident());
    }
    if (node->has_args())
        visit(node->args());
}

void Printer::visit(ulam::Ref<ulam::ast::FullTypeName> node) {
    assert(node->has_type_name());
    accept_me(node->type_name());
    if (node->has_array_dims())
        print_array_dims(node->array_dims());
    if (node->is_ref())
        _os << "&";
}

void Printer::visit(ulam::Ref<ulam::ast::ParamList> node) {
    _os << "(";
    if (do_visit(node))
        traverse_list(node);
    _os << ")";
}

void Printer::visit(ulam::Ref<ulam::ast::Param> node) {
    assert(node->has_type_name());
    visit(node->type_name());
    _os << " ";
    print_var_decl(node);
}

void Printer::visit(ulam::Ref<ulam::ast::ArgList> node) {
    _os << "(";
    if (do_visit(node))
        traverse_list(node);
    _os << ")";
}

void Printer::visit(ulam::Ref<ulam::ast::InitValue> node) {
    node->get().accept([&](auto&& ptr) { accept_me(ulam::ref(ptr)); });
}

void Printer::visit(ulam::Ref<ulam::ast::InitList> node) {
    _os << (node->is_constr_call() ? "(" : "{");
    if (do_visit(node))
        traverse_list(node);
    _os << (node->is_constr_call() ? ")" : "}");
}

void Printer::visit(ulam::Ref<ulam::ast::InitMap> node) {
    _os << "{";
    if (do_visit(node)) {
        unsigned n = 0;
        for (ulam::str_id_t key : node->keys()) {
            if (n++ > 0)
                _os << ", ";
            _os << "." << str(key) << " = ";
            auto& item_v = node->get(key);
            item_v.accept([&](auto&& ptr) { ptr->accept(*this); });
        }
    }
    _os << "}";
}

void Printer::visit(ulam::Ref<ulam::ast::Block> node) {
    if (do_visit(node)) {
        _os << "{" << nl();
        inc_lvl();
        traverse(node);
        dec_lvl();
        indent() << "}";
    }
}

void Printer::visit(ulam::Ref<ulam::ast::EmptyStmt> node) { _os << ";"; }

void Printer::visit(ulam::Ref<ulam::ast::If> node) {
    assert(node->has_cond());
    assert(node->has_if_branch());
    _os << "if (";
    accept_me(node->cond());
    _os << ")";
    if (node->has_if_branch()) {
        _os << " ";
        accept_me(node->if_branch());
    } else {
        _os << ";";
    }
    if (node->has_else_branch()) {
        _os << nl();
        indent() << "else ";
        accept_me(node->else_branch());
    }
}

void Printer::visit(ulam::Ref<ulam::ast::IfAs> node) {
    assert(node->has_ident());
    assert(node->has_type_name());
    assert(node->has_if_branch());
    _os << "if (";
    accept_me(node->ident());
    _os << " as ";
    accept_me(node->type_name());
    _os << ")";
    if (node->has_if_branch()) {
        _os << " ";
        accept_me(node->if_branch());
    } else {
        _os << ";";
    }
    if (node->has_else_branch()) {
        _os << nl();
        indent() << "else ";
        accept_me(node->else_branch());
    }
}

void Printer::visit(ulam::Ref<ulam::ast::For> node) {
    _os << "for (";
    accept_me(node->init());
    _os << " ";
    accept_me(node->cond());
    _os << "; ";
    accept_me(node->upd());
    _os << ")";
    if (node->has_body()) {
        _os << " ";
        accept_me(node->body());
    } else {
        _os << ";";
    }
}

void Printer::visit(ulam::Ref<ulam::ast::While> node) {
    assert(node->has_cond());
    _os << "while (";
    accept_me(node->cond());
    _os << ")";
    if (node->has_body()) {
        _os << " ";
        accept_me(node->body());
    } else {
        _os << ";";
    }
}

void Printer::visit(ulam::Ref<ulam::ast::Which> node) {
    assert(node->has_expr());
    _os << "which (";
    accept_me(node->expr());
    _os << ") {\n";
    for (unsigned n = 0; n < node->case_num(); ++n)
        accept_me(node->case_(n));
    indent() << "}";
}

void Printer::visit(ulam::Ref<ulam::ast::WhichCase> node) {
    if (node->is_default()) {
        indent() << "otherwise";
    } else {
        indent() << "case ";
        accept_me(node->expr());
    }
    _os << ": ";
    if (node->has_branch())
        accept_me(node->branch());
    _os << nl();
}

void Printer::visit(ulam::Ref<ulam::ast::Return> node) {
    _os << "return ";
    if (node->has_expr())
        accept_me(node->expr());
    _os << ";";
}

void Printer::visit(ulam::Ref<ulam::ast::Continue> node) { _os << "continue;"; }

void Printer::visit(ulam::Ref<ulam::ast::Break> node) { _os << "break;"; }

void Printer::visit(ulam::Ref<ulam::ast::TypeOpExpr> node) {
    assert(node->has_type_name() != node->has_expr());
    if (node->has_type_name()) {
        accept_me(node->type_name());
    } else if (node->has_expr()) {
        accept_me(node->expr());
    }
    _os << "." << ulam::ops::str(node->op());
}

void Printer::visit(ulam::Ref<ulam::ast::ParenExpr> node) {
    assert(node->has_inner());
    _os << "(";
    accept_me(node->inner());
    _os << ")";
}

void Printer::visit(ulam::Ref<ulam::ast::Cast> node) {
    assert(node->has_full_type_name());
    assert(node->expr());
    paren_l();
    _os << "(";
    accept_me(node->full_type_name());
    _os << ") ";
    accept_me(node->expr());
    paren_r();
}

void Printer::visit(ulam::Ref<ulam::ast::Ternary> node) {
    assert(node->has_cond());
    assert(node->has_if_true());
    assert(node->has_if_false());
    paren_l();
    accept_me(node->cond());
    _os << " ? ";
    accept_me(node->if_true());
    _os << " : ";
    accept_me(node->if_false());
    paren_r();
}

void Printer::visit(ulam::Ref<ulam::ast::BinaryOp> node) {
    assert(node->has_lhs());
    assert(node->has_rhs());
    paren_l();
    accept_me(node->lhs());
    _os << " " << ulam::ops::str(node->op()) << " ";
    accept_me(node->rhs());
    paren_r();
}

void Printer::visit(ulam::Ref<ulam::ast::UnaryOp> node) {
    assert(node->op() != ulam::Op::None);
    assert(node->has_arg());
    paren_l();
    if (node->op() == ulam::Op::Is) {
        // `ident is Type`
        assert(node->has_type_name());
        accept_me(node->arg());
        _os << " " << ulam::ops::str(node->op()) << " ";
        accept_me(node->type_name());
    } else {
        if (ulam::ops::is_unary_pre_op(node->op()))
            _os << ulam::ops::str(node->op());
        accept_me(node->arg());
        if (ulam::ops::is_unary_post_op(node->op()))
            _os << ulam::ops::str(node->op());
    }
    paren_r();
}

void Printer::visit(ulam::Ref<ulam::ast::ArrayAccess> node) {
    assert(node->has_array());
    assert(node->has_index());
    paren_l();
    accept_me(node->array());
    _os << "[";
    accept_me(node->index());
    _os << "]";
    paren_r();
}

void Printer::visit(ulam::Ref<ulam::ast::MemberAccess> node) {
    assert(node->has_obj());
    paren_l();
    if (node->has_base())
        accept_me(node->base());
    accept_me(node->obj());
    _os << ".";
    if (node->is_op()) {
        _os << "operator" << ulam::ops::str(node->op());
    } else {
        assert(node->has_ident());
        accept_me(node->ident());
    }
    paren_r();
}

void Printer::visit(ulam::Ref<ulam::ast::ClassConstAccess> node) {
    assert(node->has_type_name());
    assert(node->has_ident());
    paren_l();
    accept_me(node->type_name());
    _os << ".";
    accept_me(node->ident());
    paren_r();
}

bool Printer::do_visit(ulam::Ref<ulam::ast::ModuleDef> node) {
    // _os << "/* module */\n";
    return true;
}

void Printer::visit(ulam::Ref<ulam::ast::ExprStmt> node) {
    assert(node->has_expr());
    accept_me(node->expr());
    _os << ";";
}

void Printer::visit(ulam::Ref<ulam::ast::TypeDef> node) {
    _os << "typedef ";
    accept_me(node->type_name());
    _os << " ";
    accept_me(node->type_expr());
    _os << ";";
}

void Printer::visit(ulam::Ref<ulam::ast::TypeExpr> node) {
    if (node->is_ref())
        _os << "&";
    accept_me(node->ident());
    if (node->has_array_dims())
        print_array_dims(node->array_dims());
}

void Printer::traverse(ulam::Ref<ulam::ast::VarDefList> node) {
    if (node->is_const())
        _os << "constant ";
    accept_me(node->type_name());
    _os << " ";
    assert(node->def_num() > 0);
    for (unsigned n = 0; n < node->def_num(); ++n) {
        if (n > 0)
            _os << ", ";
        visit(node->def(n));
    }
    _os << ";";
}

void Printer::traverse(ulam::Ref<ulam::ast::TypeName> node) {
    accept_me(node->first());
    for (unsigned n = 1; n < node->child_num(); ++n) {
        _os << ".";
        accept_me(node->ident(n));
    }
}

bool Printer::do_visit(ulam::Ref<ulam::ast::Ident> node) {
    _os << name(node);
    return false;
}

bool Printer::do_visit(ulam::Ref<ulam::ast::TypeIdent> node) {
    _os << name(node);
    return false;
}

bool Printer::do_visit(ulam::Ref<ulam::ast::BoolLit> node) {
    _os << (node->value() ? "true" : "false");
    return false;
}

bool Printer::do_visit(ulam::Ref<ulam::ast::NumLit> node) {
    node->value().write_value(_os);
    return false;
}

bool Printer::do_visit(ulam::Ref<ulam::ast::StrLit> node) {
    std::string_view text{"<no value>"};
    ulam::str_id_t str_id = node->value().id;
    if (str_id != ulam::NoStrId) {
        auto& text_pool = ast()->ctx().text_pool();
        text = text_pool.get(node->value().id);
    }
    // TODO: escape
    _os << '"' << text << '"';
    return false;
}

void Printer::traverse_list(ulam::Ref<ulam::ast::Node> node, std::string sep) {
    for (unsigned n = 0; n < node->child_num(); ++n) {
        if (n > 0)
            _os << sep;
        accept_me(node->child(n));
    }
}

} // namespace test::ast
