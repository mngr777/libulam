#pragma once
#include <libulam/ast/ptr.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/dep_track.hpp>
#include <libulam/sema/helper.hpp>
#include <libulam/semantic/deps/symbol_id.hpp>
#include <libulam/semantic/type.hpp>

namespace ulam::sema {

class TypeResolver : public Helper {
public:
    explicit TypeResolver(
        ast::Ref<ast::Root> ast, Ref<Module> module, Scope* scope):
        Helper{ast, scope}, _module{module} {
        assert(module);
    }

    // NOTE: multiple deps can be returned for param exprs (TODO)
    Ref<Type>
    resolve(ast::Ref<ast::TypeName> type_name, SymbolIdSet* deps = nullptr);

private:
    Ref<Type>
    resolve_first(ast::Ref<ast::TypeSpec> type_spec, SymbolIdSet* deps);

    Ref<Module> _module;
};

} // namespace ulam::sema
