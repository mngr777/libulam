#include <libulam/ast/node.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/eval/funcall.hpp>
#include <libulam/semantic/fun.hpp>
#include <libulam/semantic/value.hpp>

class EvalFuncall : public ulam::sema::EvalFuncall {
public:
    using ulam::sema::EvalFuncall::EvalFuncall;

    ulam::sema::ExprRes funcall(
        ulam::Ref<ulam::ast::Node> node,
        ulam::sema::ExprRes&& callable,
        ulam::sema::ExprResList&& args);
};
