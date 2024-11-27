#include <libulam/diag.hpp>
#include <libulam/semantic/type_tpl.hpp>
#include <libulam/semantic/typed_value.hpp>
#include <libulam/sema/expr_visitor.hpp>
#include <libulam/sema/type_resolver.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>

namespace ulam::sema {

Ref<Type>
TypeResolver::resolve(ast::Ref<ast::TypeName> type_name, Scope* scope) {
    assert(type_name->first());
    auto first = resolve_first(type_name->first(), scope);
    if (!first)
        return {};
    // done?
    if (type_name->child_num() == 1)
        return first;
    // can continue?
    if (!first->is_class()) {
        diag().emit(diag::Error, type_name->first()->loc_id(), 1, "not a class");
        return {};
    }
    return {};
}

Ref<Type> TypeResolver::resolve_first(ast::Ref<ast::TypeSpec> type_spec, Scope* scope) {
    auto args_node = type_spec->args();

    // eval arguments
    TypedValueList args;
    if (args_node) {
        assert(args_node->child_num() > 0);
        bool arg_eval_failed = false;
        ExprVisitor ev{_ast, scope};
        for (unsigned n = 0; n < args_node->child_num(); ++n) {
            auto arg_node = args_node->get(n);
            ExprRes res = arg_node->accept(ev);
            const auto& value = res.value();
            if (value.is_nil() || value.rvalue()->is_unknown()) {
                diag().emit(diag::Error, arg_node->loc_id(), 1, "cannot evaluate argument");
                arg_eval_failed = true;
                continue;
            }
            args.push_back(res.move_typed_value());
        }
        if (arg_eval_failed)
            return {};
    }

    // find or make type
    Ref<Type> type{};
    if (type_spec->is_builtin()) {
        // builtin type
        auto type_id = type_spec->builtin_type_id();
        // TODO: non-primitive Atom type
        if (has_bitsize(type_id)) {
            auto tpl = program()->prim_type_tpl(type_id);
            type = tpl->type(args_node, std::move(args));
        } else {
            if (args.size() > 0) {
                assert(args_node);
                diag().emit(diag::Error, args_node->loc_id(), 1, "type is not a template");
            }
            type = program()->prim_type(type_id);
        }
    } else {
        // user type: class, alias or class tpl
        auto ident = type_spec->ident();
        auto name_id = ident->name().str_id();
        auto sym = scope->get(name_id);
        if (!sym) {
            diag().emit(diag::Error, ident->loc_id(), 1, "type not found");
            return {};
        }
        if (sym->is<Type>()) {
            // class or alias
            type = sym->get<Type>();
        } else {
            // class tpl
            assert(sym->is<TypeTpl>());
            auto tpl = sym->get<TypeTpl>();
            type = tpl->type(args_node, std::move(args));
        }
    }
    return type;
}

} // namespace ulam::sema
