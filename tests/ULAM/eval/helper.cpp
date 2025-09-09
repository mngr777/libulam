#include "./helper.hpp"
#include "./flags.hpp"
#include <cassert>

bool EvalHelper::in_main() const {
    return _env.stack_size() == 1;
}

bool EvalHelper::codegen_enabled() const {
    bool is_enabled = in_main() && !_env.has_flag(evl::NoCodegen);
    assert(!is_enabled || _env.has_flag(ulam::sema::evl::NoExec));
    return is_enabled;
}
