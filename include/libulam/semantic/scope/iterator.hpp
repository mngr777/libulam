#pragma once
#include <iterator>
#include <libulam/detail/variant.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/scope/view.hpp>
#include <libulam/str_pool.hpp>

namespace ulam {

namespace detail {

class ScopeIteratorBase {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = std::pair<str_id_t, Scope::Symbol*>;
    using pointer_type = const value_type*;
    using reference_type = value_type;
};

} // namespace detail

class BasicScopeIterator : public detail::ScopeIteratorBase {
public:
    explicit BasicScopeIterator(BasicScope& scope);
    explicit BasicScopeIterator();

    reference_type operator*() { return _cur; }
    pointer_type operator->() { return &_cur; }

    BasicScopeIterator& operator++();

    bool operator==(const BasicScopeIterator& other) const;
    bool operator!=(const BasicScopeIterator& other) const;

private:
    BasicScope* _scope;
    BasicScope::SymbolTable::iterator _it;
    value_type _cur;
};

class PersScopeIterator : public detail::ScopeIteratorBase {
public:
    explicit PersScopeIterator(PersScopeView view);
    explicit PersScopeIterator();

    reference_type operator*() { return _cur; }
    pointer_type operator->() { return &_cur; }

    PersScopeIterator& operator++();

    bool operator==(const PersScopeIterator& other) const;
    bool operator!=(const PersScopeIterator& other) const;

private:
    PersScopeView _view;
    value_type _cur;
};

class ScopeIterator
    : public detail::ScopeIteratorBase,
      private detail::Variant<BasicScopeIterator, PersScopeIterator> {
private:
    using BaseV = detail::Variant<BasicScopeIterator, PersScopeIterator>;

public:
    using BaseV::BaseV;

    reference_type operator*();
    pointer_type operator->();

    ScopeIterator& operator++();

    bool operator==(const ScopeIterator& other) const;
    bool operator!=(const ScopeIterator& other) const;
};

} // namespace ulam
