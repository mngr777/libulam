#pragma once
#include <libulam/ast/ptr.hpp>
#include <libulam/memory/ptr.hpp>

namespace ulam::ast {
class TypeName;
class TypeSpec;
} // namespace ulam::ast

namespace ulam {

class Scope;
class Type;

class Resolver {
public:
    Ref<Type> resolve(ast::Ref<ast::TypeName> type_name, Ref<Scope> scope);

    Ref<Type> resolve(ast::Ref<ast::TypeSpec> type_spec, Ref<Scope> scope);
};

} // namespace ulam
