#pragma once
#include <cstdint>
#include <filesystem>
#include <libulam/assert.hpp>
#include <list>
#include <optional>

namespace ulam {

using version_t = std::uint32_t;
constexpr version_t DefaultVersion = 5;

using Path = std::filesystem::path;
using PathList = std::list<Path>;

// Wrapper for std::optional<bool>, but not convertible to bool
class OptBool {
public:
    OptBool(bool value): _opt{value} {}
    OptBool() {}

    bool has_value() const { return _opt.has_value(); }

    bool value() const {
        ulam_assert(_opt.has_value());
        return _opt.value();
    }

    bool value_or(bool default_value) const {
        return _opt.value_or(default_value);
    }

    bool is_true() const { return _opt.has_value() && _opt.value(); }

    bool is_false() const { return _opt.has_value() && !_opt.value(); }

    void operator=(bool value) { _opt = value; }

private:
    std::optional<bool> _opt;
};

} // namespace ulam
