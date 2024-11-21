#pragma once
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/type/prim.hpp>

namespace ulam {

class IntType : public PrimType<IntId, 2, 64, 32> {};

class UnsignedType : public PrimType<UnsignedId, 1, 64, 32> {};

class BoolType : public PrimType<BoolId, 1, 64, 1> {};

class UnaryType : public PrimType<UnaryId, 1, 64, 32> {};

class BitsType : public PrimType<BitsId, 1, 4096, 8> {};

class StringType : public PrimType<StringId, 0, 0, 0> {};

} // namespace ulam
