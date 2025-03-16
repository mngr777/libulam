#pragma once
#include <cstdint>
#include <libulam/str_pool.hpp>

#define ULAM_MAX_INT_SIZE (sizeof(Integer) * 8)
#define ULAM_INT_64 1
#define ULAM_ATOM_SIZE 96

namespace ulam {

using bitsize_t = std::uint16_t;
constexpr bitsize_t NoBitsize = -1;

using array_idx_t = std::uint16_t;
constexpr array_idx_t UnknownArrayIdx = -1;

using array_size_t = array_idx_t;
constexpr array_size_t UnknownArraySize = -1;

using str_len_t = std::uint16_t;
constexpr str_len_t NoStrLen = -1;

#if ULAM_INT_64
using Integer = std::int64_t;
using Unsigned = std::uint64_t;
#else
using Integer = std::int32_t;
using Unsigned = std::uint32_t;
#endif
using Bool = bool; // TODO: remove

struct String {
    explicit String(str_id_t id = NoStrId): id{id} {}

    bool is_valid() const { return id != NoStrId; }

    bool operator==(const String& other) const {
        return id != NoStrId && id == other.id;
    }
    bool operator!=(const String& other) const { return !operator==(other); }

    str_id_t id;
};

using Datum = Unsigned; // binary representation

static_assert(sizeof(Datum) == sizeof(Integer));
static_assert(sizeof(Datum) == sizeof(Unsigned));

} // namespace ulam
