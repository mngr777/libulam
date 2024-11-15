#include <libulam/diag.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/type_init.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type.hpp>

namespace ulam::sema {

bool TypeInit::do_visit(ast::Ref<ast::ModuleDef> module) { return true; }

bool TypeInit::do_visit(ast::Ref<ast::ClassDef> class_def) {
    if (class_def->type())
        return false;
    auto name_id = class_def->name_id();
    // already defined in module?
    auto sym = scope()->get(name_id, Scope::Module);
    if (sym) {
        diag().emit(
            diag::Error, class_def->name_loc_id(), str(name_id).size(),
            "already defined");
        return false;
    }
    // add class type
    auto type = ulam::make<Class>(class_def);
    auto type_ref = ref(type);
    scope()->set(name_id, std::move(type));
    class_def->set_type(type_ref);
    return true;
}

// bool TypeInit::do_visit(ast::Ref<ast::TypeDef> type_def) {

// }

} // namespace ulam::sema
