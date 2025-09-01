#include <cassert>
#include <libulam/diag.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/eval/env.hpp>
#include <libulam/sema/init.hpp>
#include <libulam/sema/resolver.hpp>
#include <libulam/sema/visitor.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/scope/flags.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/class/prop.hpp>
#include <libulam/semantic/type/class_kind.hpp>
#include <libulam/semantic/type_tpl.hpp>
#include <libulam/semantic/var.hpp>
#include <libulam/str_pool.hpp>
#include <unordered_map>
#include <unordered_set>

#ifdef DEBUG_INIT
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[sema::Init] "
#endif
#include "src/debug.hpp"

namespace ulam::sema {

void Init::visit(Ref<ast::Root> node) {
    assert(!node->program());
    node->set_program(make<Program>(
        diag(), node->ctx().str_pool(), node->ctx().text_pool(), _sm));
    RecVisitor::visit(node);
    export_classes();
    resolve();
}

void Init::visit(Ref<ast::ModuleDef> node) {
    assert(!node->module());
    auto module = program()->add_module(node);
    node->set_module(module);
    RecVisitor::visit(node);
}

bool Init::do_visit(Ref<ast::ClassDef> node) {
    assert(pass() == Pass::Module);
    assert(!node->cls() && !node->cls_tpl());
    assert(scope()->is(scp::Module));

    // already defined?
    auto name_id = node->name_id();
    auto prev = module()->get(name_id);
    if (prev) {
        diag().error(
            node->name().loc_id(), str(name_id).size(),
            "already defined"); // TODO: say where
        return false;
    }

    if (node->has_params()) {
        assert(node->params()->child_num() > 0);
        module()->add_class_tpl(node);
    } else {
        module()->add_class(node);
    }
    module()->add_dep(ast()->ctx().str_pool().put("UrSelf")); // TODO: option
    sync_scope(node);
    return true;
}

void Init::visit(Ref<ast::TypeDef> node) {
    // visit TypeName
    assert(node->has_type_name());
    node->type_name()->accept(*this);

    auto alias_id = node->alias_id();
    // name is already in current scope?
    if (scope()->has(alias_id, true)) {
        // TODO: after types are resolves, complain if types don't match
        return;
    }

    if (scope()->is(scp::Persistent)) {
        if (scope()->is(scp::Module)) {
            module()->add_type_def(node);
        } else if (scope()->is(scp::Class | scp::ClassTpl)) {
            auto class_base = class_def()->cls_or_tpl();
            assert(class_base);
            class_base->add_type_def(node);
        }
        sync_scope(node);
    } else {
        // transient typedef (in function body)
        Ptr<UserType> type = make<AliasType>(
            program()->str_pool(), program()->builtins(), nullptr, node);
        scope()->set(alias_id, std::move(type));
    }
}

void Init::visit(Ref<ast::VarDefList> node) {
    // visit `TypeName`
    assert(node->has_type_name());
    node->type_name()->accept(*this);

    // add module/class/tpl variables/properties
    for (unsigned n = 0; n < node->def_num(); ++n) {
        auto def = node->def(n);
        auto name_id = def->name_id();

        // visit for potential type names
        if (def->has_init())
            def->init()->accept(*this);

        // already in current scope?
        if (scope()->has(name_id, true)) {
            diag().error(def, "already defined");
            continue;
        }

        if (!scope()->is(scp::Persistent))
            continue;

        // add to module/class/tpl
        if (scope()->is(scp::Module)) {
            module()->add_const(node->type_name(), def);
        } else if (scope()->is(scp::Class | scp::ClassTpl)) {
            auto cls_base = class_def()->cls_or_tpl();
            assert(cls_base);
            if (node->is_const()) {
                cls_base->add_const(node->type_name(), def);
            } else {
                cls_base->add_prop(node->type_name(), def);
            }
        }
        sync_scope(def);
    }
}

bool Init::do_visit(Ref<ast::FunDef> node) {
    assert(scope()->is(scp::Class | scp::ClassTpl));

    // get class/tpl, name
    auto cls_base = class_def()->cls_or_tpl();
    assert(cls_base);

    auto name_id = node->name_id();
    auto sym = cls_base->get(name_id);
    if (sym && !sym->is<FunSet>()) {
        diag().error(
            node->loc_id(), str(name_id).size(),
            "defined and is not a function");
        return false;
    }

    cls_base->add_fun(node);
    sync_scope(node);
    return true;
}

bool Init::do_visit(Ref<ast::TypeName> node) {
    // NOTE: type name nodes can have embedded type names in parameters, e.g.
    // `Foo(Bar.maxof)` so we still need to go deeper

    auto type_spec = node->first();
    if (type_spec->is_builtin() || type_spec->ident()->is_self())
        return true;

    // add unresolved name to module dependencies
    assert(type_spec->has_ident());
    auto name_id = type_spec->ident()->name_id();
    if (!scope()->has(name_id)) {
        debug() << "module " << module()->name() << " depends on "
                << str(name_id) << "\n";
        module()->add_dep(name_id);
    }
    return true;
}

void Init::export_classes() {
    // collect set of exporting modules for each exported symbol
    // (must be 1 per symbol)
    using ModuleSet = std::unordered_set<Ref<Module>>;
    std::unordered_map<str_id_t, ModuleSet> exporting;
    for (auto& mod : program()->modules()) {
        debug() << "exporting symbols from module " << mod->name() << "\n";
        for (auto& [name_id, sym] : *mod) {
            debug() << "module " << mod->name() << " exports " << str(name_id)
                    << "\n";
            auto [it, inserted] = exporting.emplace(name_id, ModuleSet{});
            assert(sym.is<Class>() || sym.is<ClassTpl>());
            it->second.insert(mod);
        }
    }

    // remove names defined in multiple modules
    for (auto it = exporting.begin(); it != exporting.end();) {
        auto name_id = it->first;
        auto& mods = it->second;
        if (mods.size() > 1) {
            for (auto& mod : mods) {
                auto sym = mod->get(name_id);
                assert(sym);
                auto cls_base =
                    sym->accept([&](auto cls_or_tpl) -> Ref<ClassBase> {
                        return cls_or_tpl;
                    });
                diag().error(
                    cls_base->node()->loc_id(), str(name_id).size(),
                    "defined in multiple modules");
            }
            it = exporting.erase(it);
        } else {
            ++it;
        }
    }

    // import dependencies
    for (auto& mod : program()->modules()) {
        for (auto name_id : mod->deps()) {
            auto it = exporting.find(name_id);
            if (it == exporting.end())
                continue;
            assert(it->second.size() == 1);
            auto& exporter = *it->second.begin();
            if (true || *exporter != *mod) {
                auto sym = exporter->get(name_id);
                assert(sym);
                sym->accept([&](auto cls_or_tpl) {
                    debug()
                        << "importing " << str(name_id) << " into "
                        << mod->name() << " from " << exporter->name() << "\n";
                    mod->add_import(name_id, exporter, cls_or_tpl);
                    assert(mod->scope()->has(name_id));
                });
            }
        }
    }
}

void Init::resolve() {
    auto program = ast()->program();
    EvalEnv env{program};
    env.resolver(false).resolve(program);
}

} // namespace ulam::sema
