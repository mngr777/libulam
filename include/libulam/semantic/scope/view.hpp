#pragma once
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/scope/context.hpp>
#include <libulam/semantic/scope/flags.hpp>
#include <libulam/semantic/scope/version.hpp>
#include <libulam/str_pool.hpp>
#include <utility>

namespace ulam {

class PersScope;
class PersScopeIterator;

class PersScopeView : public Scope {
public:
    explicit PersScopeView(
        PersScope* scope, ScopeVersion version = NoScopeVersion);
    PersScopeView(): Scope{}, _scope{}, _version{NoScopeVersion} {}

    void reset() { set_version(0); }
    void sync();
    std::pair<str_id_t, Symbol*> advance();

    Scope* parent(scope_flags_t flags = scp::NoFlags) override;

    operator bool() const { return _scope; }

    scope_flags_t flags() const override;

    Symbol* get(str_id_t name_id, bool current = false) override;
    Symbol* get_local(str_id_t name_id) override;

    ScopeContextProxy ctx() override;

    str_id_t last_change() const;

    ScopeVersion version() const { return _version; }
    void set_version(ScopeVersion version);
    void set_version_after(ScopeVersion version);

    ScopeIterator begin() override;
    ScopeIterator end() override;

    bool operator==(const PersScopeView& other) const;
    bool operator!=(const PersScopeView& other) const;

protected:
    Symbol* do_set(str_id_t name_id, Symbol&& symbol) override;

private:
    PersScope* scope();
    const PersScope* scope() const;

    PersScope* _scope;
    ScopeVersion _version;
};

} // namespace ulam
