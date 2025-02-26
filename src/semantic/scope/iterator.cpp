#include <libulam/semantic/scope/iterator.hpp>

namespace ulam {

PersScopeIterator::PersScopeIterator(PersScopeView view):
    _view{std::move(view)}, _cur{} {
    view.reset();
    operator++();
}

PersScopeIterator& PersScopeIterator::operator++() {
    _cur = _view.advance();
    return *this;
}

bool PersScopeIterator::operator==(const PersScopeIterator& other) {
    return (_view == other._view) || (!_cur.second && !other._cur.second);
}

bool PersScopeIterator::operator!=(const PersScopeIterator& other) {
    return !operator==(other);
}

} // namespace ulam
