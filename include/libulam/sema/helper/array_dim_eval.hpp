#pragma once
#include <libulam/sema/helper.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type.hpp>

namespace ulam::sema {

class ArrayDimEval : public Helper {
public:
    explicit ArrayDimEval(ast::Ref<ast::Root> ast, ScopeProxy scope):
        Helper{ast}, _scope{scope} {}

    std::pair<array_size_t, bool> eval(ast::Ref<ast::Expr> expr);

private:
    ScopeProxy _scope;
};

} // namespace ulam::sema
