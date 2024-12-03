#pragma once
#include <libulam/sema/visitor.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/str_pool.hpp>
#include <iostream>
#include <string_view>

namespace ulam::sema {

// TMP
class Out {
public:
    Out(Ref<Program> program): _program{program}, _os{std::cout} {}

    void print_str(str_id_t str_id) { _os << str(str_id) << "\n"; }
    void print(Ref<Scope> scope);
    void print(RecVisitor::Pass pass);
    void print(Scope::Symbol* sym);

private:
    std::string_view str(str_id_t str_id);

    Ref<Program> _program;
    std::ostream& _os;
};

} // namespace ulam::sema
