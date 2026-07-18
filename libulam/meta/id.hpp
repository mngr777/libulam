#pragma once
#include <cstdint>
#include <libulam/meta/type.hpp>

namespace ulam::meta {

enum Id : std::uint8_t {
#define FIELD(name, id, type, flags) id,
    None,
#include <libulam/meta/fields.inc.hpp>
};

} // namespace ulam::meta
