#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/type.hpp>
#include <map>

namespace ulam {
class Class;
}

namespace ulam::ast {
class TypeName;
}

namespace ulam::cls {

class Ancestor {
public:
    Ancestor(Ref<Class> cls, Ref<ast::TypeName> node): _cls{cls}, _node{node} {}

    Ref<Class> cls() { return _cls; }
    Ref<const Class> cls() const { return _cls; }

    Ref<ast::TypeName> node() { return _node; }
    Ref<const ast::TypeName> node() const { return _node; }

private:
    Ref<Class> _cls;
    Ref<ast::TypeName> _node;
};

class Ancestry {
public:
    bool add(Ref<Class> cls, Ref<ast::TypeName> node);

    bool has_base(Ref<Class> cls) const;

    auto& parents() { return _parents; }
    const auto& parents() const { return _parents; }

private:
    std::map<type_id_t, Ref<Ancestor>> _map;
    std::vector<Ref<Ancestor>> _parents;
    std::vector<Ptr<Ancestor>> _ancestors;
};

} // namespace ulam::cls
