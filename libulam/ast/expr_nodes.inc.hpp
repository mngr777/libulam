#if !defined(EXPR_NODE)
#    define EXPR_NODE(name, cls)
#    error "EXPR_NODE() macro is not defined"
#endif

EXPR_NODE("TypeOpExpr", TypeOpExpr)
EXPR_NODE("Ident", Ident)
EXPR_NODE("ParenExpr", ParenExpr)
EXPR_NODE("BinaryOp", BinaryOp)
EXPR_NODE("UnaryOp", UnaryOp)
EXPR_NODE("Cast", Cast)
EXPR_NODE("Ternary", Ternary)
EXPR_NODE("ClassName", ClassName)
EXPR_NODE("BoolLit", BoolLit)
EXPR_NODE("NumLit", NumLit)
EXPR_NODE("StrLit", StrLit)
EXPR_NODE("FunCall", FunCall)
EXPR_NODE("MemberAccess", MemberAccess)
EXPR_NODE("ClassConstAccess", ClassConstAccess)
EXPR_NODE("ArrayAccess", ArrayAccess)

#undef EXPR_NODE
