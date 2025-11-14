#include "./test_context.hpp"
#include <cassert>
#include <libulam/semantic/type/builtin/atom.hpp>
#include <libulam/semantic/value/types.hpp>
#include <utility>

namespace {

constexpr ulam::array_idx_t site_to_idx(ulam::array_idx_t site) {
    assert(site > 0);
    assert(site <= EvalTestContext::NeighborNum);
    return site - 1;
}

} // namespace

EvalTestContext::EvalTestContext(
    ulam::Builtins& builtins, ulam::LValue active_atom):
    _builtins{&builtins}, _active_atom{active_atom} {}

EvalTestContext::EvalTestContext(): _builtins{} {}

EvalTestContext::EvalTestContext(EvalTestContext&& other) {
    operator=(std::move(other));
}

EvalTestContext& EvalTestContext::operator=(EvalTestContext&& other) {
    std::swap(_builtins, other._builtins);
    std::swap(_active_atom, other._active_atom);
    std::swap(_neighbors, other._neighbors);
    return *this;
}

bool EvalTestContext::empty() const { return !_builtins; }

ulam::RValue EvalTestContext::neighbor(ulam::array_idx_t site) const {
    assert(!empty());
    const auto idx = site_to_idx(site);
    if (_neighbors[idx].empty())
        return _builtins->atom_type()->construct();
    return _neighbors[idx].copy();
}

void EvalTestContext::set_neighbor(
    ulam::array_idx_t site, ulam::RValue&& rvalue) {
    assert(!empty());
    const auto idx = site_to_idx(site);
    _neighbors[idx] = std::move(rvalue);
}
