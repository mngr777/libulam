#include <libulam/semantic/type/builtin/bits.hpp>

namespace ulam {

RValue BitsType::load(const BitVectorView data, BitVector::idx_t off) const {
    Bits val{data.view(off, bitsize()).copy()};
    return val;
}

void BitsType::store(BitVectorView data, BitVector::idx_t off, const RValue& rval) const {
    assert(off + bitsize() <= data.len());
    assert(rval.is<Bits>());
    // data.write();
}

} // namespace ulam
