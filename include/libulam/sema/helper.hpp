#pragma once
#include <cassert>
#include <libulam/ast/context.hpp>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/diag.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/str_pool.hpp>
#include <string_view>

namespace ulam::sema {

class Helper {
public:
    Helper(ast::Ref<ast::Root> ast, Scope* scope): _ast{ast}, _scope{scope} {
        assert(ast);
        assert(scope);
    }
    virtual ~Helper(){};

protected:
    ast::Ref<ast::Root> ast() { return _ast; }
    Scope* scope() { return _scope; }

    Ref<Program> program() {
        assert(_ast->program());
        return _ast->program();
    }

    Diag& diag() { return program()->diag(); }

    const std::string_view str(str_id_t str_id) const {
        return _ast->ctx().str(str_id);
    }

private:
    ast::Ref<ast::Root> _ast;
    Scope* _scope;
};

} // namespace ulam::sema
