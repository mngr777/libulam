#include <libulam/semantic/scope/iter.hpp>

namespace ulam {

// BasicScopeIter

BasicScopeIter::BasicScopeIter(BasicScope& scope):
    _scope{&scope}, _it{scope._symbols.begin()} {
    operator++();
}

BasicScopeIter::BasicScopeIter():
    _scope{}, _it{}, _cur{NoStrId, nullptr} {}

BasicScopeIter& BasicScopeIter::operator++() {
    assert(_cur.second);
    if (_scope && _it != _scope->_symbols.end()) {
        ++_it;
        _cur = {_it->first, &_it->second};
    } else {
        _cur = {NoStrId, nullptr};
    }
    return *this;
}

bool BasicScopeIter::operator==(const BasicScopeIter& other) const {
    return (_scope == other._scope) &&
           (!_scope || _cur.second == other._cur.second);
}

bool BasicScopeIter::operator!=(const BasicScopeIter& other) const {
    return !operator==(other);
}

// PersScopeIter

PersScopeIter::PersScopeIter(PersScopeView view):
    _view{std::move(view)}, _cur{} {
    view.reset();
    operator++();
}

PersScopeIter::PersScopeIter(): _view{}, _cur{NoStrId, nullptr} {}

PersScopeIter& PersScopeIter::operator++() {
    _cur = _view.advance();
    return *this;
}

bool PersScopeIter::operator==(const PersScopeIter& other) const {
    return (_view == other._view) || (!_cur.second && !other._cur.second);
}

bool PersScopeIter::operator!=(const PersScopeIter& other) const {
    return !operator==(other);
}

// ScopeIter

ScopeIter::reference_type ScopeIter::operator*() {
    return accept([&](auto& it) { return *it; });
}

ScopeIter::pointer_type ScopeIter::operator->() {
    return accept([&](auto& it) { return it.operator->(); });
}

ScopeIter& ScopeIter::operator++() {
    accept([&](auto& it) { ++it; });
    return *this;
}

bool ScopeIter::operator==(const ScopeIter& other) const {
    return BaseV::operator==(other);
}

bool ScopeIter::operator!=(const ScopeIter& other) const {
    return BaseV::operator!=(other);
}

} // namespace ulam
