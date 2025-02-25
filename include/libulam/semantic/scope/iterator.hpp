#pragma once
#include <iterator>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/scope/view.hpp>
#include <libulam/str_pool.hpp>

namespace ulam {

class PersScopeIterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = std::pair<str_id_t, Scope::Symbol*>;
    using pointer_type = const value_type*;
    using reference_type = value_type&;

    explicit PersScopeIterator(PersScopeView view):
        _view{std::move(view)}, _cur{} {
        view.reset();
        operator++();
    }

    explicit PersScopeIterator(): _view{}, _cur{NoStrId, nullptr} {}

    reference_type operator*() { return _cur; }
    pointer_type operator->() { return &_cur; }

    PersScopeIterator& operator++();

    bool operator==(const PersScopeIterator& other);
    bool operator!=(const PersScopeIterator& other);

private:
    PersScopeView _view;
    value_type _cur;
};

} // namespace ulam
