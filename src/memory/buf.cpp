#include <libulam/assert.hpp>
#include <libulam/memory/buf.hpp>

namespace ulam::mem {

BufRef BufRef::sub(std::size_t off, std::size_t size) {
    ulam_assert(off + size <= _size);
    return {_data + off, size};
}

const BufRef BufRef::sub(std::size_t off, std::size_t size) const {
    return const_cast<BufRef&>(*this).sub(off, size);
}

} // namespace ulam::mem
