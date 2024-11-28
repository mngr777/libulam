#include <libulam/diag.hpp>
#include <libulam/sema/expr_visitor.hpp>
#include <libulam/sema/helper/param_eval.hpp>
#include <libulam/sema/helper/type_resolver.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/type_tpl.hpp>
#include <libulam/semantic/typed_value.hpp>

namespace ulam::sema {

Ref<Type>
TypeResolver::resolve(ast::Ref<ast::TypeName> type_name, Scope* scope) {
    assert(type_name->first());
    auto type = resolve_first(type_name->first(), scope);
    if (!type) {
        diag().emit(
            diag::Error, type_name->first()->loc_id(), 1,
            "failed to resolve type");
        return {};
    }
    // do {
    //     AliasTypeList aliases;
    //     AliasTypeSet alias_set;
    //     assert(type->is_basic());
    //     if (type->basic()->is_alias()) {
    //         auto alias = type->basic()->as_alias();
    //         aliases.push_back(alias);
    //         alias_set.insert(alias);
    //         if (!alias->aliased()) {
    //         }
    //     }
    // } while (true); // !!

    // for (unsigned n = 1; n < type_name->child_num(); ++n) {
    //     auto ident = type_name->ident(n);
    // }

    // AliasTypeList aliases;
    // return resolve_rest(type_name, 1, scope, aliases);
    return {};
}

Ref<Type>
TypeResolver::resolve_first(ast::Ref<ast::TypeSpec> type_spec, Scope* scope) {
    auto args_node = type_spec->args();

    // eval arguments
    TypedValueList args;
    if (args_node) {
        ParamEval pe{ast()};
        auto [args, pe_success] = pe.eval(args_node, scope);
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
                diag().emit(
                    diag::Error, args_node->loc_id(), 1,
                    "type is not a template");
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
            if (args.size() > 0) {
                diag().emit(
                    diag::Error, args_node->loc_id(), 1,
                    "type is not a template");
            }
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

Ref<Type> TypeResolver::resolve_rest(
    ast::Ref<ast::TypeName> type_name,
    unsigned n,
    Scope* scope,
    AliasTypeList& aliases) {
    assert(n + 1 < type_name->child_num());
    // auto ident = type_name->get_ident(n);
    // TODO
    return {};
}

} // namespace ulam::sema
