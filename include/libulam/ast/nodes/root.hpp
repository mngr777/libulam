#pragma once
#include <libulam/ast/context.hpp>
#include <libulam/ast/node.hpp>
#include <libulam/ast/nodes/module.hpp>

namespace ulam {
class Program;
}

namespace ulam::ast {

class Root : public List<Node, ModuleDef> {
    ULAM_AST_NODE
    ULAM_AST_PTR_ATTR(Program, program)
public:
    Root();
    ~Root();

    Context& ctx() { return _ctx; }
    const Context& ctx() const { return _ctx; }

private:
    Context _ctx;
    NameIdMap<ModuleDef> _name_id_map;
};

} // namespace ulam::ast
