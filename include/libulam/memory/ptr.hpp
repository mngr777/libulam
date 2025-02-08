#pragma once
#include <memory>
#include <utility>
#include <variant>

namespace ulam {

template <typename T> using Ptr = std::unique_ptr<T>; // owning pointer
template <typename T> using Ref = T*;                 // nullable reference
template <typename T> using SPtr = std::shared_ptr<T>;

template <typename T> Ref<T> ref(Ptr<T>& ptr) { return ptr.get(); }
template <typename T> const Ref<T> ref(const Ptr<T>& ptr) { return ptr.get(); }

template <typename T, typename... Args> Ptr<T> make(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template <typename... Ts> using Variant = std::variant<Ptr<Ts>...>;

template <typename T, typename... Ts> bool is(const Variant<Ts...>& v) {
    return std::holds_alternative<Ptr<T>>(v);
}

template <typename... Ts> using RefVariant = std::variant<Ref<Ts>...>;

template <typename T, typename... Ts> bool is(const RefVariant<Ts...>& v) {
    return std::holds_alternative<Ref<T>>(v);
}

template <typename T, typename... Ts> auto as_ref(Variant<Ts...>& v) {
    return std::visit([](auto&& ptr) -> Ref<T> { return ref(ptr); }, v);
}
template <typename T, typename... Ts> auto as_ref(const Variant<Ts...>& v) {
    return std::visit([](auto&& ptr) -> const Ref<T> { return ref(ptr); }, v);
}

template <typename T> struct RefPtr {
public:
    using Type = T;

    RefPtr(Ptr<T>&& val): _value{std::move(val)} {}
    RefPtr(Ref<T> val): _value{val} {}

    bool owns() const { return std::holds_alternative<Ptr<T>>(_value); }

    Ref<T> ref() { return const_cast<Ref<T>>(std::as_const(*this).ref()); }

    Ref<const T> ref() const {
        return owns() ? ulam::ref(std::get<Ptr<T>>(_value))
                      : std::get<Ref<T>>(_value);
    }

private:
    std::variant<Ref<T>, Ptr<T>> _value;
};

} // namespace ulam
