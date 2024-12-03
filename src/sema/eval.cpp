#include <libulam/sema/eval.hpp>

namespace ulam::sema {

bool Eval::visit(ast::Ref<ast::TypeName> node) {
    return {};
}

} // namespace ulam::sema
