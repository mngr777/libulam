#pragma once
#include <cstdint>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <list>

namespace ulam {

class Fun;
class PrimType;

using conv_cost_t = std::uint8_t;

static constexpr conv_cost_t SamePrimTypeConvCost = 1;
static constexpr conv_cost_t DiffPrimTypeConvCost = 2;
static constexpr conv_cost_t AtomToElementConvCost = 2;
static constexpr conv_cost_t ClassConvCost = 3;
static constexpr conv_cost_t CastCost =
    ClassConvCost + DiffPrimTypeConvCost + 1;
static constexpr conv_cost_t ElementToAtomConvCost = 2;
static constexpr conv_cost_t ClassToBitsConvCost = 3;
static constexpr conv_cost_t BitsToClassConvCost = 3;
static constexpr conv_cost_t ClassDowncastCost = 1;
static constexpr conv_cost_t ClassUpcastCost = CastCost;
static constexpr conv_cost_t MaxConvCost = -1;

class ConvList {
public:
    using List = std::list<Ref<Fun>>;

    ConvList(): _cost{MaxConvCost} {}

    void push(Ref<Fun> fun, conv_cost_t cost);

    auto begin() const { return _list.cbegin(); }
    auto end() const { return _list.cend(); }

    std::size_t size() const { return _list.size(); }
    bool empty() const { return size() == 0; }

    conv_cost_t cost() const { return _cost; }

private:
    List _list;
    conv_cost_t _cost;
};

conv_cost_t
prim_conv_cost(Ref<const PrimType> type, Ref<const PrimType> target);

conv_cost_t
prim_cast_cost(Ref<const PrimType> type, Ref<const PrimType> target);

conv_cost_t prim_conv_cost(Ref<const PrimType> type, BuiltinTypeId bi_type_id);
conv_cost_t prim_cast_cost(Ref<const PrimType> type, BuiltinTypeId bi_type_id);

} // namespace ulam
