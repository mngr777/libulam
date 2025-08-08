#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/eval/flags.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/src_loc.hpp>
#include <libulam/str_pool.hpp>

namespace ulam::sema {

class EvalBase {
public:
    explicit EvalBase(Ref<Program> program, eval_flags_t flags = evl::NoFlags):
        _program{program}, _flags{flags} {}
    virtual ~EvalBase() {}

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

    Ref<Program> program() { return _program; }
    Diag& diag() { return _program->diag(); }
    Builtins& builtins() { return _program->builtins(); }

    UniqStrPool& str_pool() { return _program->str_pool(); }
    const UniqStrPool& str_pool() const { return _program->str_pool(); }

    UniqStrPool& text_pool() { return _program->text_pool(); }
    const UniqStrPool& text_pool() const { return _program->text_pool(); }

    bool has_flag(eval_flags_t flag) const { return _flags & flag; }
    eval_flags_t flags() const { return _flags; }

protected:
    class FlagsRaii {
    public:
        using eval_flags_t = ulam::sema::eval_flags_t;

        FlagsRaii(EvalBase& eval, eval_flags_t flags):
            _eval{eval}, _orig_flags{eval._flags} {
            _eval._flags = flags;
        }
        ~FlagsRaii() { _eval._flags = _orig_flags; }

    private:
        EvalBase& _eval;
        eval_flags_t _orig_flags;
    };

    FlagsRaii flags_raii(ulam::sema::eval_flags_t flags) {
        return {*this, flags};
    }

    bool is_true(const ExprRes& res, bool default_value = false);

private:
    Ref<Program> _program;
    eval_flags_t _flags;
};

} // namespace ulam::sema
