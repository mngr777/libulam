#if !defined(NODE)
#    define NODE(str, cls)
#    error "NODE() macro is not defined"
#endif

NODE("Module", Module)
NODE("ClassDef", ClassDef)
NODE("ClassDefBody", ClassDefBody)
NODE("TypeDef", TypeDef)
NODE("VarDefList", VarDefList)
NODE("VarDef", VarDef)
NODE("FunDef", FunDef)
NODE("Param", Param)
NODE("ParamList", ParamList)
NODE("ArgList", ArgList)
NODE("EmptyStmt", EmptyStmt)
NODE("Block", Block)
NODE("If", If)
NODE("For", For)
NODE("While", While)
NODE("Expr", Expr)
NODE("FunCall", FunCall)
NODE("ArrayAccess", ArrayAccess)
NODE("MemberAccess", MemberAccess)
NODE("TypeIdent", TypeIdent)
NODE("TypeSpec", TypeSpec)
NODE("TypeName", TypeName)
NODE("TypeOpExpr", TypeOpExpr)
NODE("Ident", Ident)
NODE("ParenExpr", ParenExpr)
NODE("BinaryOp", BinaryOp)
NODE("UnaryPreOp", UnaryPreOp)
NODE("UnaryPostOp", UnaryPostOp)
NODE("Cast", Cast)
NODE("BoolLit", BoolLit)
NODE("NumLit", NumLit)
NODE("StrLit", StrLit)
