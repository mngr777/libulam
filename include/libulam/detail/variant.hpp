#pragma once
#include <libulam/memory/ptr.hpp>
#include <utility>
#include <variant>

namespace ulam::detail {

namespace variant {
template <class... Ts> struct Overloads : Ts... {
    using Ts::operator()...;
};
template <class... Ts> Overloads(Ts...) -> Overloads<Ts...>;
} // namespace variant

// TODO: nullable/non-nullable

template <typename... Ts> class Variant {
public:
    template <typename T> explicit Variant(T&& value): _value{std::forward<T>(value)} {}
    Variant() {}
    virtual ~Variant() {}

    Variant(const Variant&) = default;
    Variant& operator=(const Variant&) = default;

    Variant(Variant&&) = default;
    Variant& operator=(Variant&&) = default;

    bool empty() const { return is<std::monostate>(); }

    std::size_t index() const { return _value.index(); }

    template <typename T> bool is() const {
        return std::holds_alternative<T>(_value);
    }

    template <typename T> T& get() { return std::get<T>(_value); }
    template <typename T> const T& get() const { return std::get<T>(_value); }

    template <typename... Vs> auto accept(Vs&&... visitors) {
        return std::visit(variant::Overloads{std::move(visitors)...}, _value);
    }
    template <typename... Vs> auto accept(Vs&&... visitors) const {
        return std::visit(variant::Overloads{std::move(visitors)...}, _value);
    }

private:
    std::variant<std::monostate, Ts...> _value;
};

template <typename... Ts>
using RefVariant = Variant<Ref<Ts>...>;

template <typename... Ts> class RefPtrVariant {
public:
    template <typename T>
    RefPtrVariant(Ptr<T>&& value): _value{std::move(value)} {}
    template <typename T> RefPtrVariant(Ref<T> value): _value{value} {}
    virtual ~RefPtrVariant() {}

    RefPtrVariant(const RefPtrVariant&) = default;
    RefPtrVariant& operator=(const RefPtrVariant&) = default;

    RefPtrVariant(RefPtrVariant&&) = default;
    RefPtrVariant& operator=(RefPtrVariant&&) = default;

    bool owns() const {
        return std::visit(
            [](auto&& value) -> bool { return value.owns(); }, _value);
    }

    std::size_t index() const { return _value.index(); }

    template <typename T> bool is() const {
        return std::holds_alternative<RefPtr<T>>(_value);
    }

    template <typename T> Ref<T> get() {
        return std::get<RefPtr<T>>(_value).ref();
    }

    template <typename T> Ref<const T> get() const {
        return std::get<RefPtr<T>>(_value).ref();
    }

    template <typename... Vs> auto accept(Vs&&... visitors) {
        return std::visit(
            [&](auto&& value) {
                return variant::Overloads{std::move(visitors)...}(value.ref());
            },
            _value);
    }

    template <typename... Vs> auto accept(Vs&&... visitors) const {
        return std::visit(
            [&](const auto&& value) {
                return variant::Overloads{std::move(visitors)...}(value.ref());
            },
            _value);
    }

private:
    std::variant<RefPtr<Ts>...> _value;
};

} // namespace ulam::detail
