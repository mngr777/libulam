#pragma once
#include <iterator>
#include <libulam/detail/variant.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/scope/view.hpp>
#include <libulam/str_pool.hpp>

namespace ulam {

namespace detail {

class ScopeIterBase {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = std::pair<str_id_t, Scope::Symbol*>;
    using pointer_type = const value_type*;
    using reference_type = value_type;
};

} // namespace detail

class BasicScopeIter : public detail::ScopeIterBase {
public:
    explicit BasicScopeIter(BasicScope& scope);
    explicit BasicScopeIter();

    reference_type operator*() { return _cur; }
    pointer_type operator->() { return &_cur; }

    BasicScopeIter& operator++();

    bool operator==(const BasicScopeIter& other) const;
    bool operator!=(const BasicScopeIter& other) const;

private:
    BasicScope* _scope;
    BasicScope::SymbolTable::iterator _it;
    value_type _cur;
};

class PersScopeIter : public detail::ScopeIterBase {
public:
    explicit PersScopeIter(PersScopeView view);
    explicit PersScopeIter();

    reference_type operator*() { return _cur; }
    pointer_type operator->() { return &_cur; }

    PersScopeIter& operator++();

    bool operator==(const PersScopeIter& other) const;
    bool operator!=(const PersScopeIter& other) const;

private:
    PersScopeView _view;
    value_type _cur;
};

class ScopeIter
    : public detail::ScopeIterBase,
      private detail::Variant<BasicScopeIter, PersScopeIter> {
private:
    using BaseV = detail::Variant<BasicScopeIter, PersScopeIter>;

public:
    using BaseV::BaseV;

    reference_type operator*();
    pointer_type operator->();

    ScopeIter& operator++();

    bool operator==(const ScopeIter& other) const;
    bool operator!=(const ScopeIter& other) const;
};

} // namespace ulam
