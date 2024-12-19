#include <libulam/sema/cast.hpp>

namespace ulam::sema {

ExprRes
Cast::cast(ExprRes&& res, Ref<ast::Node> node, Ref<Type> type, bool is_impl) {
    if (!res.ok())
        return res;
    assert(res.type());

    if (res.type() == type)
        return res;

    Value value =
        res.type()->cast(_program->diag(), node, type, res.value(), is_impl);
    ExprError error = !value.is_nil() ? ExprError::Ok : ExprError::InvalidCast;
    return {type, std::move(value), error};
}

ExprRes Cast::to_boolean(ExprRes&& res, Ref<ast::Node> node, bool is_impl) {
    auto type = _program->builtins().boolean();
    return cast(std::move(res), node, type, is_impl);
}

} // namespace ulam::sema
