#pragma once
#include <memory>

namespace ulam {

template <typename T> using Ptr = std::unique_ptr<T>; // owning pointer
template <typename T> using Ref = T*;                 // nullable reference

template <typename T> Ref<T> ref(Ptr<T>& ptr) { return ptr.get(); }
template <typename T> const Ref<T> ref(const Ptr<T>& ptr) { return ptr.get(); }

template <typename T, typename... Args> Ptr<T> make(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

} // namespace ulam
