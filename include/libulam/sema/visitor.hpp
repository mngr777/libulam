#pragma once
#include "libulam/ast/nodes/module.hpp"
#include <libulam/ast.hpp>

namespace ulam {
class Sema;
}

namespace ulam::sema {

class _Visitor : public ast::Visitor {
public:
    explicit _Visitor(Sema& sema): _sema{sema} {}

#define NODE(str, cls)                                                         \
public:                                                                        \
    virtual bool visit(ast::cls& node) override {                              \
        do_visit(node);                                                        \
        return true;                                                           \
    }                                                                          \
                                                                               \
protected:                                                                     \
    virtual bool do_visit(ast::cls& node) { return true; }
#include <libulam/ast/nodes.inc.hpp>
#undef NODE

    Sema& _sema;
};

class Visitor : public _Visitor {
public:
    explicit Visitor(Sema& sema): _Visitor{sema} {}

    virtual bool visit(ast::ModuleDef& module);
    virtual bool visit(ast::ClassDefBody& class_body);
    // TODO: blocks, loops, ...

protected:
    class InScope {
    public:
        explicit InScope(Sema& sema);
        ~InScope();

    private:
        Sema& _sema;
    };

    InScope in_scope() { return InScope{_sema}; }

    // ast::Ref<ast::ModuleDef> _module_def;
    // ast::Ref<ast::ClassDef> _class_def;
};

} // namespace ulam::sema
