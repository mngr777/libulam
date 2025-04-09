#include <libulam/ast/node.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/eval/funcall.hpp>
#include <libulam/semantic/fun.hpp>
#include <libulam/semantic/value.hpp>

class EvalFuncall : public ulam::sema::EvalFuncall {
public:
    using ulam::sema::EvalFuncall::EvalFuncall;

protected:
    ulam::sema::ExprRes do_funcall(
        ulam::Ref<ulam::ast::Node> node,
        ulam::Ref<ulam::Fun> fun,
        ulam::sema::ExprRes&& callable,
        ulam::sema::ExprResList&& args);
};
