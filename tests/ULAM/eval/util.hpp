#include <libulam/sema/expr_res.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/value.hpp>

namespace util {

inline bool can_fold(ulam::Ref<const ulam::Type> type) {
    return !type->is_ref() && !type->is_array() && !type->is_class();
}

inline bool can_fold(const ulam::sema::ExprRes& res) {
    bool is_foldable = !res.value().empty() && res.value().is_consteval() &&
                       can_fold(res.type());
    if (is_foldable) {
        res.value().with_rvalue(
            [&](const ulam::RValue& rval) { is_foldable = !rval.empty(); });
    }
    return is_foldable;
}

} // namespace util
