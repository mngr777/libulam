#pragma once
#include <libulam/detail/variant.hpp>
#include <type_traits>

#define VAR_SWITCH(v) switch (v.index())
#define VAR_CASE(v, T) case type_index<T>(v):

namespace ulam::detail {

template <std::size_t Cur, typename T, typename First, typename... Rest>
constexpr std::size_t _type_index() {
    if (std::is_same_v<T, First>())
        return Cur;
    return _type_index<Cur + 1, T, Rest...>();
}

template <typename T, typename... Ts>
constexpr std::size_t type_index(const Variant<Ts...>&) {
    return _type_index<0, T, Ts...>();
}

} // namespace ulam::details
