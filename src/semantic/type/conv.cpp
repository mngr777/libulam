#include <libulam/semantic/type/conv.hpp>
#include <libulam/semantic/type/prim.hpp>

namespace ulam {

void ConvList::push(Ref<Fun> fun, conv_cost_t cost) {
    if (cost > _cost)
        return;
    if (cost < _cost) {
        _list.clear();
        _cost = cost;
    }
    _list.push_back(fun);
}

conv_cost_t
prim_conv_cost(Ref<const PrimType> type, Ref<const PrimType> target) {
    assert(type->is_impl_castable_to(target));
    return type->is(target->bi_type_id()) ? SamePrimTypeConvCost
                                               : DiffPrimTypeConvCost;
}

conv_cost_t prim_cast_cost(Ref<const PrimType> from, Ref<const PrimType> to) {
    assert(!from->is_impl_castable_to(to));
    assert(from->is_expl_castable_to(to));
    return CastCost + (from->is(to->bi_type_id()) ? SamePrimTypeConvCost
                                                       : DiffPrimTypeConvCost);
}

conv_cost_t prim_conv_cost(Ref<const PrimType> type, BuiltinTypeId bi_type_id) {
    assert(is_prim(bi_type_id));
    assert(type->is_impl_castable_to(bi_type_id));
    return type->is(bi_type_id) ? 0 : DiffPrimTypeConvCost;
}

conv_cost_t prim_cast_cost(Ref<const PrimType> type, BuiltinTypeId bi_type_id) {
    assert(is_prim(bi_type_id));
    assert(!type->is_impl_castable_to(bi_type_id));
    assert(type->is_expl_castable_to(bi_type_id));
    return CastCost + (type->is(bi_type_id) ? 0 : DiffPrimTypeConvCost);
}

} // namespace ulam
