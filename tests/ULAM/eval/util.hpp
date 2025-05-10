#include <libulam/sema/expr_res.hpp>
#include <libulam/semantic/type.hpp>

namespace util {

inline bool can_fold(ulam::Ref<const ulam::Type> type) {
    return !type->is_ref() && !type->is_array() && !type->is_class();
}

inline bool can_fold(const ulam::sema::ExprRes& res) {
    return !res.value().empty() && res.value().is_consteval() &&
           can_fold(res.type());
}

} // namespace util
