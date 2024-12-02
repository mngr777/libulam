#pragma once
#include <libulam/ast/nodes/type.hpp>
#include <libulam/ast/ptr.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/helper.hpp>
#include <libulam/semantic/type.hpp>
#include <utility>

namespace ulam::sema {

class Resolver : Helper {
public:
    using TypeRes = std::pair<Ref<Type>, bool>;

    Resolver(ast::Ref<ast::Root> ast, Ref<Scope> scope, bool required):
        Helper{ast, scope} {}
};

} // namespace ulam::sema
