#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/type/builtin/atom.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/value/object.hpp>

namespace ulam {

RValue AtomType::load(const BitVectorView data, BitVector::size_t off) const {
    assert(false);
}

void AtomType::store(
    BitVectorView data, BitVector::size_t off, const RValue& rval) const {
    assert(false);
}

RValue AtomType::construct() const {
    // TODO
    return RValue{make_s<Object>(const_cast<AtomType*>(this))};
}

} // namespace ulam
