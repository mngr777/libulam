#if !defined(NODE)
#    define NODE(str, cls)
#    error "NODE() macro is not defined"
#endif

NODE("Expr", Expr)
NODE("Name", Name)
NODE("ParenExpr", ParenExpr)
NODE("BinaryOp", BinaryOp)
NODE("UnaryOp", UnaryOp)
NODE("Number", Number)
NODE("String", String)
