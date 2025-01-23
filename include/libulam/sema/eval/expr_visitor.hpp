#include <libulam/sema/expr_visitor.hpp>
#include <libulam/semantic/scope.hpp>

namespace ulam::sema {

class EvalVisitor;

class EvalExprVisitor : public ExprVisitor {
    friend EvalVisitor;

public:
    EvalExprVisitor(EvalVisitor& eval, Ref<Scope> scope);

protected:
    virtual ExprRes funcall(Ref<Fun> fun, TypedValueList&& args);

private:
    EvalVisitor& _eval;
};

} // namespace ulam::sema
