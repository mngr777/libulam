#pragma once
#include <libulam/memory/ptr.hpp>

namespace ulam {

class Type;
class PrimType;

#define TYPE(name, cls) class cls;
#include <libulam/semantic/types.inc.hpp>

class TypeVisitor {
public:
#define TYPE(name, cls) virtual void visit(Ref<cls> type);
#include <libulam/semantic/types.inc.hpp>

protected:
    virtual void visit_prim_default(Ref<PrimType> type);
    virtual void visit_default(Ref<Type> type);
};

} // namespace ulam
