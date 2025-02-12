#include <libulam/sema/expr_visitor.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/value/object.hpp>

namespace ulam::sema {

class EvalVisitor;

class EvalExprVisitor : public ExprVisitor {
    friend EvalVisitor;

public:
    EvalExprVisitor(EvalVisitor& eval, Ref<Scope> scope);

protected:
    ExprRes funcall(Ref<Fun> fun, SPtr<Object> obj, TypedValueList&& args) override;

private:
    EvalVisitor& _eval;
};

} // namespace ulam::sema
