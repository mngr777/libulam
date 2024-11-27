#pragma once
#include <libulam/ast/ptr.hpp>
#include <libulam/diag.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/value.hpp>

namespace ulam::ast {
class ArgList;
}

namespace ulam {

class Program;

class TypeTpl {
public:
    TypeTpl(Ref<Program> program);

    virtual Ref<Type>
    type(ast::Ref<ast::ArgList> args_node, TypedValueList&& args) = 0;

    type_id_t id() const { return _id; }

protected:
    Ref<Program> program() { return _program; }
    Ref<const Program> program() const { return _program; }
    Diag& diag();

private:
    Ref<Program> _program;
    type_id_t _id;
};

} // namespace ulam
