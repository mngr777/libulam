#pragma once
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/ast/nodes/init.hpp>
#include <libulam/detail/variant.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/eval/helper.hpp>
#include <libulam/sema/expr_res.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/value/types.hpp>

namespace ulam::sema {

class EvalVisitor;

class EvalInit : public EvalHelper {
public:
    using EvalHelper::EvalHelper;

    // {dimensions, is_array}
    std::pair<ArrayDimList, bool>
    array_dims(unsigned num, Ref<ast::InitValue> init);

    virtual ExprRes eval_init(Ref<Type> type, Ref<ast::InitValue> init, bool is_const);

protected:
    using Variant = ast::InitValue::ItemT;
    static_assert(std::is_same<Variant, ast::InitList::ItemT>());
    static_assert(std::is_same<Variant, ast::InitMap::ItemT>());

    ExprRes eval_v(Ref<Type> type, Variant& v, unsigned depth);

    virtual ExprRes
    eval_expr(Ref<Type> type, Ref<ast::Expr> expr, unsigned depth);
    virtual ExprRes
    eval_list(Ref<Type> type, Ref<ast::InitList> list, unsigned depth);
    virtual ExprRes
    eval_map(Ref<Type> type, Ref<ast::InitMap> map, unsigned depth);

    virtual ExprRes
    eval_class_list(Ref<Class> cls, Ref<ast::InitList> list, unsigned depth);

    virtual ExprRes eval_array_list(
        Ref<ArrayType> array_type, Ref<ast::InitList> list, unsigned depth);

    virtual ExprRes
    eval_class_map(Ref<Class> cls, Ref<ast::InitMap> map, unsigned depth);

    virtual ExprRes
    eval_array_list_item(Ref<Type> type, Variant& item_v, unsigned depth);

    virtual ExprRes make_array(Ref<ArrayType> array_type);
    virtual ExprRes
    array_set(ExprRes&& array, array_idx_t idx, ExprRes&& item, bool autofill);

    virtual ExprRes make_obj(Ref<Class> cls);
    virtual ExprRes obj_set(ExprRes&& obj, Ref<Prop> prop, ExprRes&& prop_res);
};

} // namespace ulam::sema
