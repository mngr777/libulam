#pragma once
#include <memory>

namespace ulam::ast {

// Owning pointer and nullable reference, aka (non-owning) pointer

template <typename N> using Ptr = std::unique_ptr<N>; // owning pointer
template <typename N> using Ref = N*;                 // nullable reference

template <typename N> Ref<N> ref(Ptr<N>& ptr) { return ptr.get(); }
template <typename N> const Ref<N> ref(const Ptr<N>& ptr) { return ptr.get(); }

template <typename N, typename... Args> Ptr<N> make(Args&&... args) {
    return std::make_unique<N>(std::forward<Args>(args)...);
}

} // namespace ulam::ast
