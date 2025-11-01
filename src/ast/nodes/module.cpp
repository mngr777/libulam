#include <libulam/ast/nodes/module.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/class_tpl.hpp>

namespace ulam::ast {

Ref<ClassBase> ClassDef::cls_or_tpl() {
    if (cls())
        return cls();
    return cls_tpl();
}

} // namespace ulam::ast
