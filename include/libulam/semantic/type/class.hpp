#pragma once
#include <libulam/semantic/type.hpp>

namespace ulam::ast {
class ClassDef;
}

namespace ulam {

class Class : public BasicType {
public:
    explicit Class(type_id_t id, Ref<ast::ClassDef> node);
    ~Class();

    Ref<ast::ClassDef> node() { return _node; }

    Ref<Scope> scope() { return ref(_scope); }

private:
    Ref<ast::ClassDef> _node;
    Ptr<Scope> _scope;
};

} // namespace ulam
