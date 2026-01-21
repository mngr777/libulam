#if !defined(FUN_OP)
#    define FUN_OP(name, op)
#    error "FUN_OP() macro is not defined"
#endif

FUN_OP("aref", ArrayAccess)
