#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/builtins.hpp>
#include <libulam/str_pool.hpp>
#include <ostream>
#include <string>

namespace ulam {
class Program;
}

namespace ulam::utils {

class Strf {
public:
    static constexpr char NoValue[] = "<no-value>";
    static constexpr char InvalidStrId[] = "<invalid-str-id>";
    static constexpr char Atom[] = "<Atom>";

    explicit Strf(Ref<Program> program);

    std::string str(Ref<Type> type, const Value& val);
    std::string str(Ref<Type> type, const LValue& lval);
    std::string str(Ref<Type> type, const RValue& rval);

    void str(std::ostream& os, Ref<Type> type, const Value& val);
    void str(std::ostream& os, Ref<Type> type, const LValue& lval);
    void str(std::ostream& os, Ref<Type> type, const RValue& rval);

protected:
    virtual void write_prim(std::ostream& os, Ref<PrimType> type, const RValue& rval);
    virtual void write_array(std::ostream& os, Ref<ArrayType> type, const RValue& rval);
    virtual void write_class(std::ostream& os, Ref<Class> cls, const RValue& rval);

    Builtins& _builtins;
    UniqStrPool& _str_pool;
    UniqStrPool& _text_pool;
};

} // namespace ulam::utils
