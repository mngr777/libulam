#include <libulam/assert.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/builtin/atom.hpp>
#include <libulam/semantic/type/builtin/bits.hpp>
#include <libulam/semantic/type/builtin/bool.hpp>
#include <libulam/semantic/type/builtin/fun.hpp>
#include <libulam/semantic/type/builtin/int.hpp>
#include <libulam/semantic/type/builtin/string.hpp>
#include <libulam/semantic/type/builtin/unary.hpp>
#include <libulam/semantic/type/builtin/unsigned.hpp>
#include <libulam/semantic/type/builtin/void.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/type/visitor.hpp>

namespace ulam {

#define TYPE(name, cls)                                                        \
    void TypeVisitor::visit(Ref<cls> type) { visit_default(type); }
#define PRIM_TYPE(name, cls)                                                   \
    void TypeVisitor::visit(Ref<cls> type) {                                   \
        visit_prim_default(type->as_prim());                                   \
    }
#include <libulam/semantic/types.inc.hpp>

void TypeVisitor::visit_prim_default(Ref<PrimType> type) {
    visit_default(type);
}

void TypeVisitor::visit_default(Ref<Type> type) { unreachable(); }

} // namespace ulam
