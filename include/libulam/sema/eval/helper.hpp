#pragma once
#include <cassert>
#include <libulam/ast/node.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/eval/flags.hpp>
#include <libulam/sema/eval/visitor.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/src_loc.hpp>
#include <libulam/str_pool.hpp>

namespace ulam::sema {

class EvalHelper {
public:
    EvalHelper(
        EvalVisitor& eval,
        Ref<Program> program,
        Ref<Scope> scope,
        eval_flags_t flags):
        _eval{eval}, _program{program}, _scope{scope}, _flags{flags} {}

protected:
    const std::string_view line_at(loc_id_t loc_id) {
        return _program->sm().line_at(loc_id);
    }
    const std::string_view line_at(Ref<ast::Node> node) {
        return line_at(node->loc_id());
    }

    const std::string_view str(str_id_t str_id) const {
        return str_pool().get(str_id);
    }
    const std::string_view text(str_id_t str_id) const {
        return text_pool().get(str_id);
    }

    EvalVisitor& eval() { return _eval; }
    Diag& diag() { return _program->diag(); }
    Builtins& builtins() { return _program->builtins(); }

    UniqStrPool& str_pool() { return _program->str_pool(); }
    const UniqStrPool& str_pool() const { return _program->str_pool(); }

    UniqStrPool& text_pool() { return _program->text_pool(); }
    const UniqStrPool& text_pool() const { return _program->text_pool(); }

    Ref<Scope> scope() { return _scope; }
    Ref<const Scope> scope() const { return _scope; }

    eval_flags_t flags() const { return _flags; }

private:
    EvalVisitor& _eval;
    Ref<Program> _program;
    Ref<Scope> _scope;
    eval_flags_t _flags;
};

} // namespace ulam::sema
