#ifndef NODE
#    define NODE(name, cls)
#    error "NODE() macro is not defined"
#endif

#ifndef EXPR_NODE
#    define EXPR_NODE NODE
#endif
#include <libulam/ast/expr_nodes.inc.hpp>

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
NODE("Cond", Cond)
NODE("If", If)
NODE("For", For)
NODE("While", While)
NODE("Which", Which)
NODE("WhichCase", WhichCase)
NODE("WhichCaseCondList", WhichCaseCondList)
NODE("WhichCaseCond", WhichCaseCond)
NODE("Return", Return)
NODE("Break", Break)
NODE("Continue", Continue)
NODE("ExprStmt", ExprStmt)
NODE("Expr", Expr)
NODE("ExprList", ExprList)
NODE("InitValue", InitValue)
NODE("InitList", InitList)
NODE("InitMap", InitMap)
NODE("TypeIdent", TypeIdent)
NODE("TypeSpec", TypeSpec)
NODE("TypeExpr", TypeExpr)
NODE("TypeName", TypeName)
NODE("TypeNameList", TypeNameList)
NODE("BaseTypeSelect", BaseTypeSelect)
NODE("FullTypeName", FullTypeName)

#undef NODE
