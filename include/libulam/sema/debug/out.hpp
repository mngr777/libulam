#pragma once
#include <iostream>
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/visitor.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/var.hpp>
#include <libulam/str_pool.hpp>
#include <string_view>

namespace ulam {
class Program;
}

namespace ulam::sema::dbg {

class Out {
public:
    Out(Ref<Program> program): _program{program}, _os{std::cout} {}

    void print(Scope& scope);
    void print(Scope::Symbol& sym);

    void print_class_registry();

private:
    const std::string_view line_at(loc_id_t loc_id);
    const std::string_view line_at(Ref<ast::Node> node);

    const std::string_view str(str_id_t str_id) const;
    const std::string_view text(str_id_t str_id) const;

    const UniqStrPool& str_pool() const;
    const UniqStrPool& text_pool() const;

    void hr();
    void hr2();

    Ref<Program> _program;
    std::ostream& _os;
};

} // namespace ulam::sema::dbg
