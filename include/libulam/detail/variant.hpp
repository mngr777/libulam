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

template <typename... Ts> class Variant {
public:
    template <typename T>
    explicit Variant(T&& value): _value{std::forward<T>(value)} {}
    Variant() {}
    virtual ~Variant() {}

    Variant(const Variant&) = default;
    Variant& operator=(const Variant&) = default;

    Variant(Variant&&) = default;
    Variant& operator=(Variant&&) = default;

    bool empty() const {
        static_assert((std::is_same_v<std::monostate, Ts> || ...));
        return is<std::monostate>();
    }

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
    std::variant<Ts...> _value;
};

template <typename... Ts>
using NullableVariant = Variant<std::monostate, Ts...>;

template <typename... Ts> using PtrVariant = Variant<Ptr<Ts>...>;

// template <typename... Ts> using RefVariant = Variant<Ref<Ts>...>;

template <typename... Ts> class RefVariant : private Variant<Ref<Ts>...> {
    using Base = Variant<Ref<Ts>...>;

public:
    template <typename T>
    explicit RefVariant(Ref<T> value): Base{std::forward<Ref<T>>(value)} {}

    bool empty() const { return Base::empty(); }

    std::size_t index() const { return Base::index(); }

    template <typename T> bool is() const {
        return Base::template is<Ref<T>>();
    }

    template <typename T> Ref<T> get() { return Base::template get<Ref<T>>(); }
    template <typename T> Ref<T> get() const {
        return Base::template get<Ref<T>>();
    }

    template <typename... Vs> auto accept(Vs&&... visitors) {
        return Base::template accept(std::move(visitors)...);
    }
    template <typename... Vs> auto accept(Vs&&... visitors) const {
        return Base::template accept(std::move(visitors)...);
    }
};

} // namespace ulam::detail
