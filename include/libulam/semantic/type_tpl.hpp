#pragma once
#include <libulam/ast/nodes/params.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/value.hpp>

namespace ulam {

class Diag;
class TypeIdGen;

class TypeTpl {
public:
    TypeTpl(type_id_t id): _id{id} {}

    virtual Ref<Type> type(
        Diag& diag,
        ast::Ref<ast::ArgList> arg_list,
        TypeIdGen& type_id_gen,
        ValueList& args) = 0;

    type_id_t id() const { return _id; }

private:
    type_id_t _id;
};

} // namespace ulam
