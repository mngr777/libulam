#pragma once
#include <libulam/sema/helper.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type.hpp>

namespace ulam::sema {

class ArrayDimEval : public Helper {
public:
    explicit ArrayDimEval(Ref<ast::Root> ast, ScopeProxy scope):
        Helper{ast}, _scope{scope} {}

    std::pair<array_size_t, bool> eval(Ref<ast::Expr> expr);

private:
    ScopeProxy _scope;
};

} // namespace ulam::sema
