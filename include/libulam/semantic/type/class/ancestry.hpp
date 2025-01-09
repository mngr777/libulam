#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/class/layout.hpp>
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
    Ancestor(Ref<Class> cls, Ref<ast::TypeName> node, cls::data_off_t data_off):
        _cls{cls}, _node{node}, _data_off{data_off} {}

    Ref<Class> cls() { return _cls; }
    Ref<const Class> cls() const { return _cls; }

    Ref<ast::TypeName> node() { return _node; }
    Ref<const ast::TypeName> node() const { return _node; }

    cls::data_off_t data_off() const { return _data_off; }

private:
    Ref<Class> _cls;
    Ref<ast::TypeName> _node;
    cls::data_off_t _data_off;
};

class Ancestry {
public:
    bool add(Ref<Class> cls, Ref<ast::TypeName> node);

    bool is_base(Ref<Class> cls) const;

    auto& parents() { return _parents; }
    const auto& parents() const { return _parents; }

    auto& ancestors() { return _ancestors; }
    const auto& ancestors() const { return _ancestors; }

private:
    std::map<type_id_t, Ref<Ancestor>> _map;
    std::vector<Ref<Ancestor>> _parents;
    std::vector<Ptr<Ancestor>> _ancestors;
    cls::data_off_t _cur_data_off{0};
};

} // namespace ulam::cls
