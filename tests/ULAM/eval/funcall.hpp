#include <libulam/ast/node.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/eval/funcall.hpp>
#include <libulam/semantic/fun.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/str_pool.hpp>

class EvalFuncall : public ulam::sema::EvalFuncall {
public:
    using ulam::sema::EvalFuncall::EvalFuncall;

protected:
    virtual ulam::sema::ExprRes funcall_callable(
        ulam::Ref<ulam::ast::Node> node,
        ulam::Ref<ulam::Fun> fun,
        ulam::sema::ExprRes&& callable,
        ulam::sema::ExprResList&& args);

    virtual ulam::sema::ExprRes funcall_obj(
        ulam::Ref<ulam::ast::Node> node,
        ulam::Ref<ulam::Fun> fun,
        ulam::sema::ExprRes&& obj,
        ulam::sema::ExprResList&& args);

    std::string arg_data(const ulam::sema::ExprResList& args);
};
