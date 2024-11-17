#pragma once
#include <cstdint>
#include <libulam/semantic/type.hpp>
#include <libulam/memory/ptr.hpp>
#include <unordered_map>

namespace ulam {

using bitsize_t = std::uint16_t;

class PrimType : public BaseType {
public:
    PrimType(type_id_t id, bitsize_t max_bitsize): BaseType{id} {}

    bitsize_t max_bitsize() const { return _max_bitsize; }

private:
    bitsize_t _max_bitsize;
    // std::unordered_map<bitsize_t, Ptr<PrimType>>;
};

} // namespace ulam
