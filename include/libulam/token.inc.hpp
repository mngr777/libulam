#if !defined(TOK)
#  define TOK(str, type)
#  error TOK() macro is not defined
#endif

TOK("ulam", Ulam)
TOK("use", Use)
TOK("load", Load)

TOK("if", If)
TOK("else", Else)
TOK("for", For)
TOK("while", While)
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

TOK("#", Sharp)
TOK("$", Dollar)
TOK("(", ParenL)
TOK(")", ParenR)
TOK("[", BracketL)
TOK("]", BracketR)
TOK("{", BraceL)
TOK("}", BraceR)
TOK(";", Semicol)
TOK("?", Quest)
TOK(":", Colon)

TOK("Void", VoidT)
TOK("Int", IntT)
TOK("Unsigned", Unsigned)
TOK("Bool", BoolT)
TOK("Unary", UnaryT)
TOK("Bits", BitsT)
TOK("String", StringT)

TOK("+", Plus)
TOK("-", Minus)
TOK("*", Ast)
TOK("/", Slash)
TOK("%", Percent)
TOK("++", PlusPlus)
TOK("--", MinusMinus)
TOK("==", EqualEqual)
TOK("!=", ExclEqual)
TOK(">", Greater)
TOK("<", Less)
TOK(">=", GreaterEq)
TOK("<=", LessEqual)
TOK("!", Excl)
TOK("&&", AmpAmp)
TOK("||", PipePipe)
TOK("~", Tilde)
TOK("&", Amp)
TOK("|", Pipe)
TOK("^", Caret)
TOK("<<", LessLess)
TOK(">>", GreaterGreater)
TOK("=", Equal)
TOK("+=", PlusEqual)
TOK("-=", MinusEqual)
TOK("*=", AstEqual)
TOK("/=", SlashEqual)
TOK("%=", PercentEqual)
TOK("&=", AmpEqual)
TOK("|=", PipeEqual)
TOK("^=", CaretEqual)
TOK("<<=", LessLessEqual)
TOK(">>=", GreaterGreaterEqual)
TOK(".", Dot)
TOK(",", Comma)
TOK("...", Ellipsis)

TOK(nullptr, TypeIdent)
TOK(nullptr, Name)
TOK(nullptr, Number)
TOK(nullptr, String)
TOK(nullptr, InvalidAtKeyword) // @something
TOK(nullptr, UnexpectedChar)
TOK(nullptr, Eof)

