#include <libulam/ast/nodes/module.hpp>
#include <libulam/ast/context.hpp>
#include <utility>

namespace ulam::ast {

void Module::add_class_def(Ptr<ClassDef>&& class_def) {
    if (_scope.get(class_def->name_id()))
        _ctx->diag().emit(diag::Error, 0, 0, "type already defined");
    auto type = make<Class>(ref(class_def));
    add(std::move(class_def));
}

} // namespace ulam::ast
