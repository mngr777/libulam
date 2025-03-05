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

    bool has_module(str_id_t name_id) const;
    void add_module(Ptr<ModuleDef>&& mod);

private:
    Context _ctx;
    NameIdMap<ModuleDef> _name_id_map;
};

} // namespace ulam::ast
