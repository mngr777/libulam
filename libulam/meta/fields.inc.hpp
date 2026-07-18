#if !defined(FIELD)
#    define FIELD(name, id, type, flags)
#    error FIELD() macro is not defined
#endif

FIELD("symbol", Symbol, String, ::ulam::meta::Field::NoFlags)
FIELD("sa", SeeAlso, String, ::ulam::meta::Field::IsMulti)

#undef FIELD
