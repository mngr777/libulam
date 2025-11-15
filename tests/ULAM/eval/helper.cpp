#include "./helper.hpp"
#include "./flags.hpp"
#include "./native.hpp"
#include <cassert>

bool EvalHelper::in_main() const { return _env.stack_size() == 1; }

bool EvalHelper::codegen_enabled() const {
    bool is_enabled = in_main() && !_env.has_flag(evl::NoCodegen);
    assert(!is_enabled || _env.has_flag(ulam::sema::evl::NoExec));
    return is_enabled;
}

ulam::sema::ExprRes EvalHelper::call_native(
    ulam::Ref<ulam::ast::Node> node,
    ulam::Ref<ulam::Fun> fun,
    ulam::LValue self,
    ulam::sema::ExprResList&& args) {
    return EvalNative{_env}.call(node, fun, self, std::move(args));
}
