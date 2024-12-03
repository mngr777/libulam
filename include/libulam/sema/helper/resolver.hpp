#pragma once
#include <libulam/ast/nodes/type.hpp>
#include <libulam/ast/ptr.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/helper.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/symbol_id.hpp>
#include <utility>

namespace ulam::sema {

class Resolver : Helper {
public:
    using TypeRes = std::pair<Ref<Type>, bool>;

    Resolver(ast::Ref<ast::Root> ast, Ref<Scope> scope):
        Helper{ast} {}

    Ref<Type> resolve_type(ast::Ref<ast::TypeName> type_name, Ref<Scope> scope);
};

} // namespace ulam::sema
