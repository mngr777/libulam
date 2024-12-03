#include <libulam/sema/eval.hpp>
#include <libulam/sema/helper/param_eval.hpp>

namespace ulam::sema {

bool Eval::do_visit(ast::Ref<ast::TypeDef> node) { return {}; }

bool Eval::do_visit(ast::Ref<ast::VarDef> node) { return {}; }

bool Eval::do_visit(ast::Ref<ast::FunDef> node) { return {}; }

Ref<Type> Eval::resolve_type_def(ast::Ref<ast::TypeDef> node, Ref<Scope> scope) {
    auto alias = node->alias_type();
    if (!alias->aliased()) {
        auto type = resolve_type_name(node->type_name(), scope);
        if (type)
            alias->set_aliased(type);
        // else: complain, mark node?
    }
    return alias;
}

Ref<Type> Eval::resolve_type_name(ast::Ref<ast::TypeName> node, Ref<Scope> scope) {
    Ref<Type> type{};
    auto type_spec = node->first();
    if (type_spec->type()) {
        type = type_spec->type();
    } else if (type_spec->type_tpl()) {
        auto tpl = type_spec->type_tpl();
        ParamEval pe{ast()}; // TODO: default param values, def scope
        auto [args, success] = pe.eval(type_spec->args(), scope);
        type = tpl->type(type_spec->args(), std::move(args));
        type_spec->set_type(type);
    }
    // TODO
    return {};
}

} // namespace ulam::sema
