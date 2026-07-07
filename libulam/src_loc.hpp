#pragma once
#include <cstdint>
#include <libulam/assert.hpp>

namespace ulam {

using loc_id_t = std::uint32_t;
using src_id_t = std::uint16_t;
using linum_t = std::uint32_t;
using chr_t = std::uint32_t;

constexpr loc_id_t NoLocId = -1;
constexpr src_id_t NoSrcId = -1;
constexpr linum_t NoLinum = 0;
constexpr chr_t NoChr = 0;

class SrcLoc {
public:
    SrcLoc(src_id_t src_id, linum_t linum, chr_t chr):
        _src_id{src_id}, _linum{linum}, _chr{chr} {
        ulam_assert(src_id != NoSrcId);
        ulam_assert(linum != NoLinum);
        ulam_assert(chr != NoChr);
    }

    SrcLoc() {}

    bool empty() const { return _src_id == NoSrcId; }

    src_id_t src_id() const { return _src_id; }
    linum_t linum() const { return _linum; }
    chr_t chr() const { return _chr; }

private:
    src_id_t _src_id{NoSrcId};
    linum_t _linum{NoLinum};
    chr_t _chr{NoChr};
};

} // namespace ulam
