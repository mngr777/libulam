#if !defined(FIELD)
#    define FIELD(name, id, type)
#    error FIELD() macro is not defined
#endif

FIELD("symbol", Symbol, String)

#undef FIELD
