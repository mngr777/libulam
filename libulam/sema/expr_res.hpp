#pragma once
#include <any>
#include <cstdint>
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/expr_error.hpp>
#include <libulam/semantic/typed_value.hpp>
#include <libulam/semantic/value.hpp>
#include <list>
#include <type_traits>
#include <utility>

namespace ulam {
class Type;
}

namespace ulam::sema {

class ExprRes {
private:
    template <typename T>
    using EnablePtrIfPtr =
        typename std::enable_if<std::is_pointer_v<T>, T>::type;

    template <typename T>
    using EnableRefIfNotPtr =
        typename std::enable_if<std::negation_v<std::is_pointer<T>>, T&>::type;

public:
    using flags_t = std::uint16_t;
    static constexpr flags_t NoFlags = 0;
    static constexpr flags_t Self = 1;
    static constexpr flags_t Super = 1 << 1;
    static constexpr flags_t Last = 1 << 2;

    using Data = std::any;

    ExprRes(TypedValue&& tv):
        _typed_value{std::move(tv)}, _error{ExprError::Ok}, _flags{0} {}

    ExprRes(Ref<Type> type, Value&& value): ExprRes{{type, std::move(value)}} {}

    ExprRes(ExprError error = ExprError::Nil): _error{error}, _flags{0} {}

    ExprRes(ExprRes&&) = default;
    ExprRes& operator=(ExprRes&&) = default;

    ExprRes copy() const;

    ExprRes derived(TypedValue&& tv, bool keep_all_flags = false);
    ExprRes derived(Ref<Type> type, Value&& val, bool keep_all_flags = false);

    bool ok() const { return _error == ExprError::Ok; }
    bool is_nil() const { return _error == ExprError::Nil; }
    operator bool() const { return ok(); }

    bool is_self() const;
    void set_is_self(bool is_self);

    bool is_super() const;
    void set_is_super(bool is_super);

    Ref<Type> type() const { return _typed_value.type(); }
    const Value& value() const { return _typed_value.value(); }
    const TypedValue& typed_value() const { return _typed_value; }

    TypedValue move_typed_value();

    Value move_value() { return _typed_value.move_value(); }

    ExprError error() const { return _error; }

    bool has_flag(flags_t flag) const { return _flags & flag; }
    void set_flag(flags_t flag) { _flags |= flag; }
    void uns_flag(flags_t flag) { _flags &= ~flag; }

    flags_t flags() const { return _flags; }
    void set_flags(flags_t flags) { _flags = flags; };

    bool has_data() const { return _data.has_value(); }

    Data& data() { return _data; }
    const Data& data() const { return _data; }

    void set_data(Data&& data) { _data = std::move(data); }

    Data move_data() {
        Data data;
        return std::exchange(_data, std::move(data));
    }

    void uns_data() { _data.reset(); }

    // return reference if T is not a pointer
    template <typename T> EnableRefIfNotPtr<T> data_as() {
        ulam_assert(has_data());
        return *std::any_cast<T>(&_data);
    }

    template <typename T> const EnableRefIfNotPtr<T> data_as() const {
        return const_cast<ExprRes&>(*this).data_as<T>();
    }

    // return pointer if T is pointer
    template <typename T> EnablePtrIfPtr<T> data_as() {
        ulam_assert(has_data());
        return std::any_cast<T>(&_data);
    }

    template <typename T> const EnablePtrIfPtr<T> data_as() const {
        return const_cast<ExprRes&>(*this).data_as<T>();
    }

    template <typename T> void set_data_as(T&& data) {
        _data = std::move(data);
    }

private:
    TypedValue _typed_value;
    ExprError _error;
    flags_t _flags;
    Data _data;
};

class ExprResList {
public:
    ExprResList() {}

    ExprResList(ExprResList&&) = default;
    ExprResList& operator=(ExprResList&&) = default;

    bool ok() const { return _list.empty() || _list.back(); }
    operator bool() const { return ok(); }

    ExprError error() {
        ulam_assert(!empty());
        return _list.back().error();
    }

    void push_back(ExprRes&& res) {
        ulam_assert(ok());
        _list.push_back(std::move(res));
    }

    ExprRes pop_front() {
        ulam_assert(!empty());
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

    // TODO: iterator
    TypedValueRefList typed_value_refs() const;

    bool is_consteval() const;

private:
    std::list<ExprRes> _list;
};

using ExprResPair = std::pair<ExprRes, ExprRes>;

} // namespace ulam::sema
