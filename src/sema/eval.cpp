#include <libulam/sema/eval.hpp>
#include <libulam/sema/helper/param_eval.hpp>

#define ULAM_DEBUG
#define ULAM_DEBUG_PREFIX "[sema::Eval] "
#include "src/debug.hpp"

namespace ulam::sema {

bool Eval::do_visit(Ref<ast::TypeDef> node) {
    resolve_type_def(node, scope());
    return RecVisitor::do_visit(node);
}

bool Eval::do_visit(Ref<ast::VarDef> node) { return {}; }

bool Eval::do_visit(Ref<ast::FunDef> node) { return {}; }

Ref<Type>
Eval::resolve_type_def(Ref<ast::TypeDef> node, Ref<Scope> scope) {
    auto alias = node->alias_type();
    if (!alias->aliased()) {
        auto type = resolve_type_name(node->type_name(), scope);
        if (type)
            alias->set_aliased(type);
        // else: complain, mark node?
    }
    return alias;
}

Ref<Type>
Eval::resolve_type_name(Ref<ast::TypeName> node, Ref<Scope> scope) {
    Ref<Type> type{};
    auto type_spec = node->first();
    if (type_spec->type()) {
        type = type_spec->type();
    } else if (type_spec->cls_tpl()) {
        auto tpl = type_spec->cls_tpl();
        // TODO: resolve tpl params
        ParamEval pe{ast()}; // TODO: default param values
        auto [args, success] = pe.eval(type_spec->args(), scope);
        type = tpl->type(diag(), type_spec->args(), std::move(args));
        type_spec->set_type(type);
    } else {
        return {};
    }
    unsigned n = 0;
    do {
        auto ident = node->ident(n);
        auto canon = type->canon();
        if (!canon) {
            diag().emit(
                diag::Error, ident->loc_id(),
                str(ident->name().str_id()).size(), "cannot be resolved");
            return {};
        }
        if (++n == node->child_num())
            return type;

        // in A(x).B.C, A(x) and A(x).B must resolve to classes
        if (!canon->is_class()) {
            diag().emit(
                diag::Error, ident->loc_id(),
                str(ident->name().str_id()).size(), "not a class");
            return {};
        }

    } while (true);
    return {};
}

} // namespace ulam::sema
