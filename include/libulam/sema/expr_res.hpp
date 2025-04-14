#pragma once
#include <algorithm>
#include <any>
#include <cassert>
#include <cstdint>
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/expr_error.hpp>
#include <libulam/semantic/typed_value.hpp>
#include <libulam/semantic/value.hpp>
#include <list>
#include <utility>

namespace ulam {
class Type;
}

namespace ulam::sema {

class ExprRes {
public:
    using flags_t = std::uint16_t;

    ExprRes(TypedValue&& tv):
        _typed_value{std::move(tv)}, _error{ExprError::Ok}, _flags{0} {}

    ExprRes(Ref<Type> type, Value&& value):
        ExprRes{{type, std::move(value)}} {}

    ExprRes(ExprError error = ExprError::NotImplemented):
        _error{error}, _flags{0} {}

    ExprRes(ExprRes&&) = default;
    ExprRes& operator=(ExprRes&&) = default;

    ExprRes copy() const {
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

    ExprRes derived(TypedValue&& tv) {
        ExprRes res{std::move(tv)};
        res._data = _data;
        res._flags = _flags;
        return res;
    }

    ExprRes derived(Ref<Type> type, Value&& val) {
        return derived({type, std::move(val)});
    }

    bool ok() const { return _error == ExprError::Ok; }
    operator bool() const { return ok(); }

    Ref<Type> type() const { return _typed_value.type(); }
    const Value& value() const { return _typed_value.value(); }
    const TypedValue& typed_value() const { return _typed_value; }

    TypedValue move_typed_value() {
        TypedValue tv;
        std::swap(tv, _typed_value);
        return tv;
    }

    Value move_value() { return _typed_value.move_value(); }

    ExprError error() const { return _error; }

    bool has_flag(flags_t flag) const { return _flags & flag; }
    void set_flag(flags_t flag) { _flags |= flag; }
    void uns_flag(flags_t flag) { _flags &= ~flag; }

    flags_t flags() const { return _flags; }
    void set_flags(flags_t flags) { _flags = flags; };

    bool has_data() const { return _data.has_value(); }

    template <typename T> T data() const {
        assert(has_data());
        return std::any_cast<T>(_data);
    }

    template <typename T, typename V> T data(V&& def) const {
        if (has_data())
            return std::any_cast<T>(_data);
        return T{std::forward<V>(def)};
    }

    template <typename T> void set_data(T data) { _data = std::move(data); }

    std::any move_data() {
        std::any data;
        std::swap(_data, data);
        return data;
    }

    void uns_data() { _data.reset(); }

private:
    TypedValue _typed_value;
    ExprError _error;
    flags_t _flags;
    std::any _data;
};

class ExprResList {
public:
    bool ok() const { return _list.empty() || _list.back(); }
    operator bool() const { return ok(); }

    ExprError error() {
        assert(!empty());
        return _list.back().error();
    }

    void push_back(ExprRes&& res) { _list.push_back(std::move(res)); }

    ExprRes pop_front() {
        assert(!empty());
        auto res = std::move(_list.front());
        _list.pop_front();
        return res;
    }

    auto begin() { return _list.begin(); }
    auto begin() const { return _list.cbegin(); }

    auto end() { return _list.end(); }
    auto end() const { return _list.end(); }

    std::size_t size() const { return _list.size(); }
    bool empty() const { return _list.empty(); }

    TypedValueRefList typed_value_refs() const {
        TypedValueRefList list;
        for (const auto& res : _list)
            list.push_back(std::cref(res.typed_value()));
        return list;
    }

private:
    std::list<ExprRes> _list;
};

} // namespace ulam::sema
