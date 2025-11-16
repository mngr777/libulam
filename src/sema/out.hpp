#pragma once
#include <iostream>
#include <libulam/sema/visitor.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/var.hpp>
#include <libulam/str_pool.hpp>
#include <string_view>

namespace ulam::sema {

class Out {
public:
    Out(Ref<Program> program): _program{program}, _os{std::cout} {}

    void print(Scope& scope);
    void print(Scope::Symbol& sym);

    void print(RecVisitor::Pass pass);
    void print(Scope::Symbol* sym);
    void print(Ref<Type> type, bool canon = false);
    void print(Ref<Var> var);

private:
    const std::string_view line_at(loc_id_t loc_id) {
        return _program->src_man().line_at(loc_id);
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

    std::string_view str(str_id_t str_id);

    Ref<Program> _program;

    UniqStrPool& str_pool() { return _program->str_pool(); }
    const UniqStrPool& str_pool() const { return _program->str_pool(); }

    UniqStrPool& text_pool() { return _program->text_pool(); }
    const UniqStrPool& text_pool() const { return _program->text_pool(); }

    void hr();
    void hr2();

    std::ostream& _os;
};

} // namespace ulam::sema
