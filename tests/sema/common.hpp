#include <libulam/ast.hpp>
#include <string>

ulam::Ptr<ulam::ast::Root> analyze(std::string text);
ulam::Ptr<ulam::ast::Root> analyze_and_print(std::string text);
