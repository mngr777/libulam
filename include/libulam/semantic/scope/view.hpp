#pragma once
#include <libulam/semantic/scope.hpp>
#include <libulam/str_pool.hpp>
#include <utility>

namespace ulam {

class PersScope;
class PersScopeIterator;

class PersScopeView : public Scope {
public:
    PersScopeView(Ref<PersScope> scope, ScopeVersion version);
    PersScopeView(): Scope{}, _scope{}, _version{NoScopeVersion} {}

    void reset() { set_version(0); }
    void sync();
    std::pair<str_id_t, Symbol*> advance();

    Ref<Scope> parent(scope_flags_t flags = scp::NoFlags) override;
    Ref<const Scope> parent(scope_flags_t flags = scp::NoFlags) const override;

    Ref<Class> self_cls() override;

    operator bool() const { return _scope; }

    scope_flags_t flags() const override;

    Symbol* get(str_id_t name_id, bool current = false) override;

    str_id_t last_change() const;

    ScopeVersion version() const override { return _version; }
    void set_version(ScopeVersion version) override;
    void set_version_after(ScopeVersion version) override;

    Ptr<PersScopeView> view(ScopeVersion version) override;
    Ptr<PersScopeView> view() override;

    PersScopeIterator begin();
    PersScopeIterator end();

    bool operator==(const PersScopeView& other) const;
    bool operator!=(const PersScopeView& other) const;

protected:
    Symbol* do_set(str_id_t name_id, Symbol&& symbol) override;

private:
    Ref<PersScope> scope();
    Ref<const PersScope> scope() const;

    Ref<PersScope> _scope;
    ScopeVersion _version;
};

} // namespace ulam
