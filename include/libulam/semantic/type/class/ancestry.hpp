#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/value/types.hpp>
#include <map>

namespace ulam {
class Class;
}

namespace ulam::ast {
class TypeName;
}

namespace ulam::cls {

class Ancestry;

class Ancestor {
    friend Ancestry;

public:
    Ancestor(Ref<Class> cls, Ref<ast::TypeName> node):
        _cls{cls},
        _node{node},
        _is_parent{false},
        _data_off{NoBitsize},
        _size_added{NoBitsize} {}

    Ref<Class> cls() const { return _cls; }

    Ref<ast::TypeName> node() { return _node; }
    Ref<const ast::TypeName> node() const { return _node; }

    bool is_parent() const { return _is_parent; }

    bool has_data_off() const;
    bitsize_t data_off() const;

    bool has_size_added() const;
    bitsize_t size_added() const;

private:
    void set_is_parent(bool is_parent) { _is_parent = is_parent; }

    void set_data_off(bitsize_t data_off);
    void set_size_added(bitsize_t size);
    void add_dep_added(Ref<Ancestor> anc);

    Ref<Class> _cls;
    Ref<ast::TypeName> _node;
    bool _is_parent;
    bitsize_t _data_off; // NOTE: offset is relative to start of inherited data
                         // section
    bitsize_t _size_added;
    // dependencies pulled in first time by current ancestor,
    std::list<Ref<Ancestor>> _deps_added;
};

class Ancestry {
public:
    bool add(Ref<Class> cls, Ref<ast::TypeName> node);
    void init();

    bool is_base(Ref<const Class> cls) const;

    Ref<Ancestor> base(str_id_t name_id);

    bitsize_t data_off(Ref<const Class> cls) const;

    auto& parents() { return _parents; }
    const auto& parents() const { return _parents; }

    auto& ancestors() { return _ancestors; }
    const auto& ancestors() const { return _ancestors; }

private:
    std::pair<Ref<Ancestor>, bool>
    do_add(Ref<Class> cls, Ref<ast::TypeName> node);

    std::map<type_id_t, Ref<Ancestor>> _map;
    std::map<str_id_t, Ref<Ancestor>> _name_id_map;
    std::vector<Ref<Ancestor>> _parents;
    std::vector<Ref<Ancestor>> _ancestors;
    std::vector<Ptr<Ancestor>> _ancestor_ptrs;
};

} // namespace ulam::cls
