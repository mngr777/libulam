#include <algorithm>
#include <cassert>
#include <libulam/sema/expr_res.hpp>

namespace ulam::sema {

namespace {
constexpr ExprRes::flags_t NonStickyFlags = ExprRes::Self;
}

// ExprRes

ExprRes ExprRes::copy() const {
    ExprRes res;
    if (ok()) {
        res = {type(), value().copy()};
    } else {
        res = {error()};
    }
    res._data = _data;
    res._flags = _flags & ~NonStickyFlags;
    return res;
}

ExprRes ExprRes::derived(TypedValue&& tv, bool keep_all_flags) {
    ExprRes res{std::move(tv)};
    res._data = _data;
    res._flags = keep_all_flags ? _flags : _flags & ~NonStickyFlags;
    return res;
}

ExprRes ExprRes::derived(Ref<Type> type, Value&& val, bool keep_all_flags) {
    return derived({type, std::move(val)}, keep_all_flags);
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

bool ExprResList::is_consteval() const {
    return std::all_of(begin(), end(), [](const auto& arg) {
        return arg.value().is_consteval();
    });
}

} // namespace ulam::sema
