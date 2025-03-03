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
TOK("break", Break)
TOK("continue", Continue)

TOK("element", Element)
TOK("quark", Quark)
TOK("transient", Transient)
TOK("union", Union)

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

TOK("self", Self)

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

TOK("Int", IntT)
TOK("Unsigned", UnsignedT)
TOK("Bool", BoolT)
TOK("Unary", UnaryT)
TOK("Bits", BitsT)
TOK("Atom", AtomT)
TOK("Void", VoidT)
TOK("String", StringT)

TOK("minof", MinOf)
TOK("maxof", MaxOf)
TOK("sizeof", SizeOf)
TOK("lengthof", LengthOf)
TOK("classidof", ClassIdOf)
TOK("constantof", ConstantOf)
TOK("positionof", PositionOf)
TOK("atomof", AtomOf)
TOK("InstanceOf", InstanceOf)

TOK("is", Is)
TOK("as", As)

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
TOK(">=", GreaterEqual)
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
TOK(".", Period)
TOK(",", Comma)
TOK("...", Ellipsis)

TOK(nullptr, Comment)
TOK(nullptr, MlComment)
TOK(nullptr, TypeIdent)
TOK(nullptr, BuiltinTypeIdent)
TOK(nullptr, Ident)
TOK(nullptr, Number)
TOK(nullptr, String)
TOK(nullptr, InvalidAtKeyword)
TOK(nullptr, UnexpectedChar)
TOK(nullptr, Eof)
