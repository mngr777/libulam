#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/value.hpp>

namespace ulam::ast {
class ArgList;
}

namespace ulam {

class Diag;
class TypeIdGen;

class TypeTpl {
public:
    TypeTpl(TypeIdGen& id_gen): _id_gen{id_gen} {}

    virtual Ref<Type>
    type(Diag& diag, Ref<ast::ArgList> args_node, TypedValueList&& args) = 0;

protected:
    TypeIdGen& id_gen() { return _id_gen; }

private:
    TypeIdGen& _id_gen;
};

} // namespace ulam
