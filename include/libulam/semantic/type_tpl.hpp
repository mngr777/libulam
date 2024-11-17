#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/ast/nodes/params.hpp>
#include <libulam/semantic/type.hpp>

namespace ulam {

class Diag;
class TypeIdGen;

class TypeTpl {
public:
    TypeTpl(type_id_t id): _id{id} {}

    virtual Ref<Type> type(Diag& diag, TypeIdGen& type_id_gen, ast::Ref<ast::ArgList> args) = 0;

    type_id_t id() const { return _id; }

private:
    type_id_t _id;
};

} // namespace ulam
