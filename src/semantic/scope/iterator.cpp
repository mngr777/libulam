#include <libulam/semantic/scope/iterator.hpp>

namespace ulam {

// BasicScopeIterator

BasicScopeIterator::BasicScopeIterator(BasicScope& scope):
    _scope{&scope}, _it{scope._symbols.begin()} {
    operator++();
}

BasicScopeIterator::BasicScopeIterator():
    _scope{}, _it{}, _cur{NoStrId, nullptr} {}

BasicScopeIterator& BasicScopeIterator::operator++() {
    assert(_cur.second);
    if (_scope && _it != _scope->_symbols.end()) {
        ++_it;
        _cur = {_it->first, &_it->second};
    } else {
        _cur = {NoStrId, nullptr};
    }
    return *this;
}

bool BasicScopeIterator::operator==(const BasicScopeIterator& other) const {
    return (_scope == other._scope) &&
           (!_scope || _cur.second == other._cur.second);
}

bool BasicScopeIterator::operator!=(const BasicScopeIterator& other) const {
    return !operator==(other);
}

// PersScopeIterator

PersScopeIterator::PersScopeIterator(PersScopeView view):
    _view{std::move(view)}, _cur{} {
    view.reset();
    operator++();
}

PersScopeIterator::PersScopeIterator(): _view{}, _cur{NoStrId, nullptr} {}

PersScopeIterator& PersScopeIterator::operator++() {
    _cur = _view.advance();
    return *this;
}

bool PersScopeIterator::operator==(const PersScopeIterator& other) const {
    return (_view == other._view) || (!_cur.second && !other._cur.second);
}

bool PersScopeIterator::operator!=(const PersScopeIterator& other) const {
    return !operator==(other);
}

// ScopeIterator

ScopeIterator::reference_type ScopeIterator::operator*() {
    return accept([&](auto& it) { return *it; });
}

ScopeIterator::pointer_type ScopeIterator::operator->() {
    return accept([&](auto& it) { return it.operator->(); });
}

ScopeIterator& ScopeIterator::operator++() {
    accept([&](auto& it) { ++it; });
    return *this;
}

bool ScopeIterator::operator==(const ScopeIterator& other) const {
    return BaseV::operator==(other);
}

bool ScopeIterator::operator!=(const ScopeIterator& other) const {
    return BaseV::operator!=(other);
}

} // namespace ulam
