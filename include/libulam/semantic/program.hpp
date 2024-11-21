#pragma once
#include <libulam/ast/ptr.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/module.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/type_id_gen.hpp>
#include <list>
#include <unordered_map>

namespace ulam::ast {
class Root;
}

namespace ulam {

class Program {
public:
    Program(ast::Ref<ast::Root> ast);

    Ref<Type> builtin_type(BuiltinTypeId id);

    auto& modules() { return _modules; }

    void add_module(Ptr<Module>&& module) {
        _modules.push_back(std::move(module));
    }

    type_id_t next_type_id() { return _type_id_gen.next(); }
    TypeIdGen& type_id_gen() { return _type_id_gen; }

private:
    ast::Ref<ast::Root> _ast;
    TypeIdGen _type_id_gen;
    std::unordered_map<BuiltinTypeId, Ptr<Type>> _builtin_types;
    std::list<Ptr<Module>> _modules;
};

} // namespace ulam
