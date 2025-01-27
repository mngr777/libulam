#include <libulam/sema/eval/except.hpp>
#include <utility>

namespace ulam {

ExprRes EvalExceptReturn::move_res() {
    ExprRes res;
    std::swap(res, _res);
    return res;
}

} // namespace ulam
