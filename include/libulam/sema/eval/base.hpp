#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/eval/flags.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/src_loc.hpp>
#include <libulam/str_pool.hpp>

namespace ulam::sema {

class EvalVisitor;

class EvalBase {
public:
    explicit EvalBase(Ref<Program> program): _program{program} {}
    virtual ~EvalBase() {}

    const std::string_view line_at(loc_id_t loc_id) {
        return _program->sm().line_at(loc_id);
    }
    const std::string_view line_at(Ref<ast::Node> node) {
        return line_at(node->loc_id());
    }

    const str_id_t str_id(const std::string_view str) const {
        return str_pool().id(str);
    }
    const std::string_view str(str_id_t str_id) const {
        return str_pool().get(str_id);
    }
    const std::string_view text(str_id_t str_id) const {
        return text_pool().get(str_id);
    }

    Ref<Program> program() { return _program; }
    Diag& diag() { return _program->diag(); }
    Builtins& builtins() { return _program->builtins(); }

    UniqStrPool& str_pool() { return _program->str_pool(); }
    const UniqStrPool& str_pool() const { return _program->str_pool(); }

    UniqStrPool& text_pool() { return _program->text_pool(); }
    const UniqStrPool& text_pool() const { return _program->text_pool(); }

    bool is_true(const ExprRes& res, bool default_value = false);

private:
    Ref<Program> _program;
};

} // namespace ulam::sema
