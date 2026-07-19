#ifndef TYPE
#    define TYPE(name, cls)
#    error "TYPE() macro is not defined"
#endif

#ifndef PRIM_TYPE
#    define PRIM_TYPE TYPE
#endif
#include <libulam/semantic/prim_types.inc.hpp>

TYPE("alias", AliasType)
TYPE("array", ArrayType)
TYPE("ref", RefType)
TYPE("atom", AtomType)
TYPE("fun", FunType)
TYPE("string", StringType)
TYPE("void", VoidType)
TYPE("class", Class)

#undef TYPE
