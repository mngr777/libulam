#include <libulam/sema/eval/base.hpp>
#include <libulam/semantic/type/builtin/bool.hpp>

namespace ulam::sema {

OptBool EvalBase::is_true(const ExprRes& res) {
    ulam_assert(res.type()->is(BoolId));
    OptBool is_truth;
    if (res.value().has_rvalue()) {
        res.value().with_rvalue([&](const auto& rval) {
            is_truth = builtins().bool_type()->is_true(rval);
        });
    }
    return is_truth;
}

} // namespace ulam::sema
