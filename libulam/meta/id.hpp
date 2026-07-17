#pragma once
#include <cstdint>
#include <libulam/meta/type.hpp>

namespace ulam::meta {

enum Id : std::uint8_t {
#define FIELD(name, id, type) id,
    None,
#include <libulam/meta/field.inc.hpp>
#undef FIELD
};

} // namespace ulam::meta
