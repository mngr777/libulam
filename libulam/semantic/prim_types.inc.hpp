#ifndef PRIM_TYPE
#    define PRIM_TYPE(name, cls)
#    error "PRIM_TYPE() macro is not defined"
#endif

PRIM_TYPE("bits", BitsType)
PRIM_TYPE("bool", BoolType)
PRIM_TYPE("int", IntType)
PRIM_TYPE("unary", UnaryType)
PRIM_TYPE("unsigned", UnsignedType)

#undef PRIM_TYPE
