#include <libulam/ast/nodes/module.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>

namespace ulam::ast {

Root::Root() {}
Root::~Root() {}

ClassDef::ClassDef(
    ClassKind kind,
    ast::Str name,
    Ptr<ParamList>&& params,
    Ptr<TypeNameList>&& ancestors):
    Tuple{std::move(params), std::move(ancestors), make<ClassDefBody>()},
    Named{name},
    _kind{kind} {}


} // namespace ulam::ast
