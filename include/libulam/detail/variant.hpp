#pragma once
#include <variant>

namespace ulam::detail {

template <typename... Ts> class Variant {
public:
    template <typename T> Variant(T&& value): _value{std::forward<T>(value)} {}
    Variant() {}
    virtual ~Variant() {}

    Variant(Variant&&) = default;
    Variant& operator=(Variant&&) = default;

    bool empty() const { return is<std::monostate>(); }

    template <typename T> bool is() const {
        return std::holds_alternative<T>(_value);
    }

    template <typename T> T& get() { return std::get<T>(_value); }
    template <typename T> const T& get() const { return std::get<T>(_value); }

    template <typename V> void accept(V&& visitor) {
        std::visit(visitor, _value);
    }

private:
    std::variant<std::monostate, Ts...> _value;
};

} // namespace ulam::detail
