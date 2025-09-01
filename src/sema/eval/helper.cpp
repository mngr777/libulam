#include <libulam/sema/eval/cast.hpp>
#include <libulam/sema/eval/env.hpp>
#include <libulam/sema/eval/except.hpp>
#include <libulam/sema/eval/helper.hpp>
#include <libulam/sema/eval/visitor.hpp>
#include <libulam/semantic/type/builtin/bool.hpp>

namespace ulam::sema {

EvalHelper::EvalHelper(EvalEnv& env): EvalBase{env.program()}, _env{env} {}

eval_flags_t EvalHelper::has_flag(eval_flags_t flag) const {
    return flags() & flag;
}

eval_flags_t EvalHelper::flags() const { return _env.flags(); }

Scope* EvalHelper::scope() { return _env.scope(); }

} // namespace ulam::sema
