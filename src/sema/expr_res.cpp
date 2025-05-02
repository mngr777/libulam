#include <algorithm>
#include <cassert>
#include <libulam/sema/expr_res.hpp>

namespace ulam::sema {

// ExprRes

ExprRes ExprRes::copy() const {
    ExprRes res;
    if (ok()) {
        res = {type(), value().copy()};
    } else {
        res = {error()};
    }
    res._data = _data;
    res._flags = _flags;
    return res;
}

ExprRes ExprRes::derived(TypedValue&& tv) {
    ExprRes res{std::move(tv)};
    res._data = _data;
    res._flags = _flags;
    return res;
}

ExprRes ExprRes::derived(Ref<Type> type, Value&& val) {
    return derived({type, std::move(val)});
}

TypedValue ExprRes::move_typed_value() {
    assert(!is_nil());
    TypedValue tv;
    std::swap(tv, _typed_value);
    _error = ExprError::Nil;
    return tv;
}

// ExprResList

TypedValueRefList ExprResList::typed_value_refs() const {
    TypedValueRefList list;
    for (const auto& res : _list)
        list.push_back(std::cref(res.typed_value()));
    return list;
}

} // namespace ulam::sema
