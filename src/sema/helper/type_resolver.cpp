#include "libulam/semantic/module.hpp"
#include "libulam/str_pool.hpp"
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
TypeResolver::resolve(ast::Ref<ast::TypeName> type_name, SymbolIdSet* deps) {
    assert(type_name->first());
    auto type = resolve_first(type_name->first(), deps);
    if (!type)
        return {};

    unsigned n = 0;
    while (true) {
        auto ident = type_name->ident(n);
        // alias type?
        if (type->basic()->is_alias()) {
            auto alias = type->basic()->as_alias();
            if (!alias->aliased()) {
                if (deps) {
                    // add dependency
                    module_id_t module_id = NoModuleId;
                    if (n == 0) {
                        // module-local alias
                        module_id = _module->id();
                    }
                    deps->emplace(module_id, alias->name_id(), NoStrId);
                } else {
                    // complain
                    diag().emit(
                        diag::Error, ident->loc_id(), 1,
                        "alias is not resolved");
                }
                return {};
            }
        }
        // done?
        if (++n == type_name->child_num())
            return type;

        // class typedef
        if (type->basic()->is_alias()) {
            assert(type->canon());
            type = type->canon();
        }
        // is this a class?
        if (!type->basic()->is_class()) {
            diag().emit(
                diag::Error, ident->loc_id(), 1, "not a class");
            return {};
        }
        // get typedef member
        ident = type_name->ident(n);
        auto cls = type->basic()->as_class();
        auto sym = cls->get(ident->name().str_id());
        if (!sym) {
            if (deps) {
                // add dependency
                deps->insert(cls->name_id(), ident->name().str_id());
            } else {
                // complain
                diag().emit(
                    diag::Error, ident->loc_id(), 1, "class does not have a typedef");
            }
            return {};
        }
        assert(sym->is<Type>());
        type = sym->get<Type>();
        // loop back to check for unresolved typedef
    }
}

Ref<Type> TypeResolver::resolve_first(
    ast::Ref<ast::TypeSpec> type_spec, SymbolIdSet* deps) {
    auto args_node = type_spec->args();

    // eval arguments
    // TODO: track dependencies in arg exprs
    TypedValueList args;
    if (args_node) {
        ParamEval pe{ast(), scope()};
        auto [args, pe_success] = pe.eval(args_node);
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
        auto sym = scope()->get(name_id);
        if (!sym) {
            // not found
            if (deps) {
                // add to dependency list
                deps->emplace(NoModuleId, name_id, NoStrId);
            } else {
                // complain
                diag().emit(diag::Error, ident->loc_id(), 1, "type not found");
            }
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

} // namespace ulam::sema
