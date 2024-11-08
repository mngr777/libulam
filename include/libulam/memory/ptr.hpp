#pragma once
#include <memory>
#include <variant>

namespace ulam {

template <typename T> using Ptr = std::unique_ptr<T>; // owning pointer
template <typename T> using Ref = T*;                 // nullable reference

template <typename T> Ref<T> ref(Ptr<T>& ptr) { return ptr.get(); }
template <typename T> const Ref<T> ref(const Ptr<T>& ptr) { return ptr.get(); }

template <typename T, typename... Args> Ptr<T> make(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template <typename... Ns> using Variant = std::variant<Ptr<Ns>...>;

template <typename T, typename... Ts> bool is(const Variant<Ts...>& v) {
    return std::holds_alternative<Ptr<T>>(v);
}

template <typename T, typename... Ts> auto as_ref(Variant<Ts...>& v) {
    return std::visit([](auto&& ptr) -> Ref<T> { return ref(ptr); }, v);
}
template <typename T, typename... Ts> auto as_ref(const Variant<Ts...>& v) {
    return std::visit([](auto&& ptr) -> const Ref<T> { return ref(ptr); }, v);
}

} // namespace ulam
