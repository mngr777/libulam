#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/ast/nodes/params.hpp>
#include <libulam/semantic/type.hpp>
#include <string>
#include <unordered_map>

namespace ulam {

class TypeTpl {
public:
    TypeTpl(ast::Ref<ast::ParamList> params): _params{params} {}

    Ref<Type> inst(type_id_t type_id, ast::Ref<ast::ArgList> args);

    auto params() { return _params; }

protected:
    virtual Ptr<Type> make(type_id_t type_id, ast::Ref<ast::ArgList> args) = 0;

private:
    ast::Ref<ast::ParamList> _params;
    std::unordered_map<std::string, Ptr<Type>> _types;
};

} // namespace ulam
