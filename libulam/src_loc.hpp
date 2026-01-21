#pragma once
#include <cstdint>

namespace ulam {

using loc_id_t = std::uint32_t;
using src_id_t = std::uint16_t;
using linum_t = std::uint32_t;
using chr_t = std::uint32_t;

constexpr loc_id_t NoLocId = -1;

class SrcLoc {
public:
    SrcLoc(src_id_t src_id, const char* ptr, linum_t linum, chr_t chr):
        _src_id{src_id}, _ptr{ptr}, _linum{linum}, _chr{chr} {}

    src_id_t src_id() const { return _src_id; }
    const char* ptr() const { return _ptr; } // TMP
    linum_t linum() const { return _linum; }
    chr_t chr() const { return _chr; }

private:
    src_id_t _src_id;
    const char* _ptr;
    linum_t _linum;
    chr_t _chr;
};

} // namespace ulam
