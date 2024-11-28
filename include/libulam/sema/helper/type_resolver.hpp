#pragma once
#include <libulam/ast/ptr.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/helper.hpp>
#include <libulam/semantic/type.hpp>
#include <list>
#include <set>

namespace ulam::sema {

class TypeResolver : public Helper {
public:
    explicit TypeResolver(ast::Ref<ast::Root> ast): Helper{ast} {}

    Ref<Type> resolve(ast::Ref<ast::TypeName> type_name, Scope* scope);

private:
    using AliasTypeList = std::list<Ref<AliasType>>;
    using AliasTypeSet = std::set<Ref<AliasType>>;

    Ref<Type> resolve_first(ast::Ref<ast::TypeSpec> type_spec, Scope* scope);

    Ref<Type> resolve_rest(
        ast::Ref<ast::TypeName> type_name,
        unsigned n,
        Scope* scope,
        AliasTypeList& aliases);
};

} // namespace ulam::sema
