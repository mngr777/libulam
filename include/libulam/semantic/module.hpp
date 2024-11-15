#pragma once
#include <libulam/ast/ptr.hpp>
#include <libulam/memory/ptr.hpp>

namespace ulam::ast {
class ModuleDef;
}

namespace ulam {

class Scope;

class Module {
public:
    Module(ast::Ref<ast::ModuleDef> node);

    Ref<Scope> scope() { return ref(_scope); }

private:
    ast::Ref<ast::ModuleDef> _node;
    Ptr<Scope> _scope;
};

} // namespace ulam
