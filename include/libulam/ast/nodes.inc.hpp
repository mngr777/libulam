#if !defined(NODE)
#    define NODE(str, cls)
#    error "NODE() macro is not defined"
#endif

NODE("Module", Module)
NODE("ClassDef", ClassDef)
NODE("TypeDef", TypeDef)
NODE("VarDef", VarDef)
NODE("FunDef", FunDef)
NODE("Expr", Expr)
NODE("Name", Name)
NODE("ParenExpr", ParenExpr)
NODE("BinaryOp", BinaryOp)
NODE("UnaryOp", UnaryOp)
NODE("Number", Number)
NODE("String", String)
