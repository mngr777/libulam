#if !defined(TOK)
#  define TOK(str, type)
#  error TOK() macro is not defined
#endif

#if defined(TOK_SEL_KEYWORD) || defined(TOK_SEL_ALL)
TOK("ulam", Ulam)
TOK("use", Use)

TOK("if", If)
TOK("else", Else)
TOK("return", Return)

TOK("element", Element)
TOK("quark", Quark)
TOK("union", Union)
TOK("transient", Transient)

TOK("typedef", Typedef)
TOK("operator", Operator)

TOK("virtual", Virtual)
TOK("native", Native)
TOK("@Override", Override)

TOK("constant", Constant)
TOK("parameter", Parameter)
TOK("local", Local)

TOK("true", True)
TOK("false", False)
#endif

#if defined(TOK_SEL_BUILTIN_TYPE) || defined(TOK_SEL_ALL)
TOK("Void", VoidT)
TOK("Int", IntT)
TOK("Unsigned", Unsigned)
TOK("Bool", BoolT)
TOK("Unary", UnaryT)
TOK("Bits", BitsT)
TOK("String", StringT)
#endif

#if defined(TOK_SEL_OP) || defined(TOK_SEL_ALL)
TOK("+", Add)
TOK("-", Sub)
TOK("*", Mult)
TOK("/", Div)
TOK("%", Mod)
TOK("++", Inc)
TOK("--", Dec)
TOK("==", Equal)
TOK("!=", NotEqual)
TOK(">", Greater)
TOK("<", Less)
TOK(">=", GreaterEq)
TOK("<=", LessEq)
TOK("!", Not)
TOK("&&", And)
TOK("||", Or)
TOK("~", NotBw)
TOK("&", AndBw)
TOK("|", OrBw)
TOK("^", XorBw)
TOK("<<", ShiftLeft)
TOK(">>", ShiftRight)
TOK("[]", Square)
TOK("=", Assign)
TOK("+=", AssignAdd)
TOK("-=", AssignSub)
TOK("*=", AssignMult)
TOK("/=", AssignDiv)
TOK("%=", AssignMod)
TOK("&=", AssignAndBw)
TOK("|=", AssignOrBw)
TOK("^=", AssignXorBw)
TOK("<<=", AssignShiftLeft)
TOK(">>=", AssignShiftRight)
TOK(".", Dot)
TOK("&", Ref)
TOK(",", Comma)
#endif

#if defined(TOK_SEL_PUNCT) || defined(TOK_SEL_ALL)
TOK("(", ParenOpen)
TOK(")", ParenClose)
TOK("[", BracketOpen)
TOK("]", BracketClose)
TOK("{", BraceOpen)
TOK("}", BraceClose)
TOK(";", SemiColon)
TOK("?", Question) // ??
TOK(":", Colon) // ??
#endif

#if defined(TOK_SEL_ALL) // everything else
TOK(nullptr, Name)
TOK(nullptr, Number)
TOK(nullptr, String)
TOK(nullptr, PlusSign)  // +/- signs are a special case
TOK(nullptr, MinusSign) // handled separately by the lexer
TOK(nullptr, InvalidFuncSpec) // @something
TOK(nullptr, None)
#endif
