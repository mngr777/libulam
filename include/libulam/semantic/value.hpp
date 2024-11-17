#pragma once
#include <cstdint>
#include <string>
#include <variant>
#include <libulam/semantic/value/bits.hpp>
#include <libulam/semantic/value/object.hpp>

namespace ulam {

using Integer = std::int64_t;
using Unsigned = std::uint64_t;
using String = std::string;

class Value {
public:
    Value() {}

    bool is_unknown() const { return is<std::monostate>(); }

    template <typename T> bool is() const {
        return std::holds_alternative<T>(_value);
    }

    template <typename T> T& get() { return std::get<T>(_value); }
    template <typename T> const T& get() const { return std::get<T>(_value); }

    template <typename T> void set(T&& value) { _value = value; }

private:
    std::variant<std::monostate, Integer, Unsigned, Bits, String> _value;
};

} // namespace ulam
