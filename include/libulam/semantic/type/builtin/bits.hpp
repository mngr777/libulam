#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/expr_res.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/type/prim.hpp>

namespace ulam::ast {
class BinaryOp;
}

namespace ulam {

class BitsType : public _PrimType<BitsId, 1, 4096, 8> {
public:
    BitsType(TypeIdGen& id_gen, Ref<PrimTypeTpl> tpl, bitsize_t bitsize):
        _PrimType{id_gen, tpl, bitsize} {}
};

using BitsTypeTpl = _PrimTypeTpl<BitsType>;

} // namespace ulam
