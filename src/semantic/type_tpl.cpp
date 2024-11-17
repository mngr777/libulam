#include <libulam/semantic/type_tpl.hpp>

namespace ulam {

Ref<Type> TypeTpl::inst(type_id_t type_id, ast::Ref<ast::ArgList> args) {
    auto type = make(type_id, args);
    auto type_ref = ref(type);
    // TODO: add to map
    return type_ref;
}

} // namespace ulam
