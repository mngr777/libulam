#pragma once
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/prim.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <map>

namespace ulam {

class Builtins {
public:
    Builtins(TypeIdGen& id_gen);

    Ref<PrimTypeTpl> prim_type_tpl(BuiltinTypeId id);

    Ref<PrimType> prim_type(BuiltinTypeId id);
    Ref<PrimType> prim_type(BuiltinTypeId id, bitsize_t size);

    Ref<Type> type(BuiltinTypeId id);

    Ref<Type> boolean();

private:
    std::map<BuiltinTypeId, Ptr<PrimTypeTpl>> _prim_type_tpls;
    std::map<BuiltinTypeId, Ptr<PrimType>> _prim_types;
    std::map<BuiltinTypeId, Ptr<Type>> _other_types;
};

} // namespace ulam
