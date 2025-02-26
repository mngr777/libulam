#include <libulam/semantic/type/builtin/atom.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/value/atom.hpp>

namespace ulam {

RValue AtomType::load(const BitVectorView data, BitVector::size_t off) const {
    Atom val{data.view(off, bitsize()).copy()};
    return RValue{std::move(val)};
}

void AtomType::store(
    BitVectorView data, BitVector::size_t off, const RValue& rval) const {
    assert(rval.is<Atom>());
    data.write(off, rval.get<Atom>().bits().view());
}

RValue AtomType::construct() const { return RValue{Atom()}; }

} // namespace ulam
