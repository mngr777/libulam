#if !defined(NODE)
#    define NODE(str, cls)
#    error "NODE() macro is not defined"
#endif

NODE("Root", Root)
NODE("ModuleDef", ModuleDef)
NODE("ClassDef", ClassDef)
NODE("ClassDefBody", ClassDefBody)
NODE("TypeDef", TypeDef)
NODE("VarDefList", VarDefList)
NODE("VarDef", VarDef)
NODE("FunDef", FunDef)
NODE("FunRetType", FunRetType)
NODE("FunDefBody", FunDefBody)
NODE("Param", Param)
NODE("ParamList", ParamList)
NODE("ArgList", ArgList)
NODE("EmptyStmt", EmptyStmt)
NODE("Block", Block)
NODE("If", If)
NODE("IfAs", IfAs)
NODE("For", For)
NODE("While", While)
NODE("Return", Return)
NODE("Break", Break)
NODE("Continue", Continue)
NODE("ExprStmt", ExprStmt)
NODE("Expr", Expr)
NODE("ExprList", ExprList)
NODE("FunCall", FunCall)
NODE("ArrayAccess", ArrayAccess)
NODE("MemberAccess", MemberAccess)
NODE("TypeIdent", TypeIdent)
NODE("TypeSpec", TypeSpec)
NODE("TypeExpr", TypeExpr)
NODE("TypeName", TypeName)
NODE("TypeNameList", TypeNameList)
NODE("TypeOpExpr", TypeOpExpr)
NODE("Ident", Ident)
NODE("ParenExpr", ParenExpr)
NODE("BinaryOp", BinaryOp)
NODE("UnaryOp", UnaryOp)
NODE("Cast", Cast)
NODE("BoolLit", BoolLit)
NODE("NumLit", NumLit)
NODE("StrLit", StrLit)
