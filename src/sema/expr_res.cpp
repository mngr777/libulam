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

bool ExprRes::is_self() const { return has_flag(Self); }

void ExprRes::set_is_self(bool is_self) {
    if (is_self) {
        set_flag(Self);
    } else {
        uns_flag(Self);
    }
}

bool ExprRes::is_super() const { return has_flag(Super); }

void ExprRes::set_is_super(bool is_super) {
    if (is_super) {
        set_flag(Super);
    } else {
        uns_flag(Super);
    }
}

TypedValue ExprRes::move_typed_value() {
    assert(!is_nil());
    _error = ExprError::Nil;
    return std::exchange(_typed_value, {});
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
