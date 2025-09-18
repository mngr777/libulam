#include <libulam/sema/eval/base.hpp>
#include <libulam/sema/eval/visitor.hpp>
#include <libulam/semantic/type/builtin/bool.hpp>

namespace ulam::sema {

bool EvalBase::is_true(const ExprRes& res, bool default_value) {
    assert(res.type()->is(BoolId));
    bool is_truth = default_value;
    if (!res.value().empty()) {
        res.value().with_rvalue([&](const auto& rval) {
            is_truth = builtins().bool_type()->is_true(rval);
        });
    }
    return is_truth;
}

} // namespace ulam::sema
