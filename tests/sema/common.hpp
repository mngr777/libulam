#include <libulam/ast.hpp>
#include <string>

ulam::Ptr<ulam::ast::Root>
analyze(const std::string& text, const std::string& module_name);

ulam::Ptr<ulam::ast::Root>
analyze_and_print(const std::string& text, const std::string& module_name);
