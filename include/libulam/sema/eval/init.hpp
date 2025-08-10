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
#include <libulam/semantic/var/base.hpp>

namespace ulam::sema {

class EvalVisitor;

class EvalInit : public ScopedEvalHelper {
public:
    using ScopedEvalHelper::ScopedEvalHelper;

    virtual ExprRes eval_init(Ref<VarBase> var, Ref<ast::InitValue> init);

protected:
    using Variant = ast::InitValue::ItemT;
    static_assert(std::is_same<Variant, ast::InitList::ItemT>());
    static_assert(std::is_same<Variant, ast::InitMap::ItemT>());

    ExprRes
    eval_v(Ref<VarBase> var, Ref<Type> type, Variant& v, unsigned depth);

    virtual ExprRes eval_expr(
        Ref<VarBase> var, Ref<Type> type, Ref<ast::Expr> expr, unsigned depth);

    virtual ExprRes cast_expr_res(
        Ref<VarBase> var,
        Ref<Type> type,
        Ref<ast::Expr> expr,
        ExprRes&& res,
        unsigned depth);

    virtual ExprRes eval_list(
        Ref<VarBase> var,
        Ref<Type> type,
        Ref<ast::InitList> list,
        unsigned depth);

    virtual ExprRes eval_map(
        Ref<VarBase> var,
        Ref<Type> type,
        Ref<ast::InitMap> map,
        unsigned depth);

    virtual ExprRes eval_class_list(
        Ref<VarBase> var,
        Ref<Class> cls,
        Ref<ast::InitList> list,
        unsigned depth);

    virtual ExprRes eval_array_list(
        Ref<VarBase> var,
        Ref<ArrayType> array_type,
        Ref<ast::InitList> list,
        unsigned depth);

    virtual ExprRes eval_class_map(
        Ref<VarBase> var,
        Ref<Class> cls,
        Ref<ast::InitMap> map,
        unsigned depth);

    virtual ExprRes eval_array_list_item(
        Ref<VarBase> var, Ref<Type> type, Variant& item_v, unsigned depth);

    virtual ExprRes make_array(Ref<VarBase> var, Ref<ArrayType> array_type);

    virtual ExprRes array_set(
        Ref<VarBase> var,
        ExprRes&& array,
        array_idx_t idx,
        ExprRes&& item,
        bool autofill,
        unsigned depth);

    virtual ExprRes make_obj(Ref<VarBase> var, Ref<Class> cls);

    virtual ExprRes construct_obj(
        Ref<VarBase> var,
        Ref<Class> cls,
        Ref<ast::InitList> arg_list,
        ExprResList&& args);

    virtual ExprRes obj_set(
        Ref<VarBase> var,
        ExprRes&& obj,
        Ref<Prop> prop,
        ExprRes&& prop_res,
        unsigned depth);
};

} // namespace ulam::sema
