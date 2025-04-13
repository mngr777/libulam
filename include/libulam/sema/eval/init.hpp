#pragma once
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/ast/nodes/init.hpp>
#include <libulam/detail/variant.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/sema/eval/helper.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/value/types.hpp>

namespace ulam::sema {

class EvalVisitor;

class EvalInit : public EvalHelper {
public:
    using EvalRes = std::pair<Value, bool>;

    using EvalHelper::EvalHelper;

    // {dimensions, is_array}
    std::pair<ArrayDimList, bool>
    array_dims(unsigned num, Ref<ast::InitValue> init);

    virtual EvalRes eval_init(Ref<Type> type, Ref<ast::InitValue> init);

protected:
    using Variant = ast::InitValue::ItemT;
    static_assert(std::is_same<Variant, ast::InitList::ItemT>());
    static_assert(std::is_same<Variant, ast::InitMap::ItemT>());

    EvalRes eval_v(Ref<Type> type, Variant& v, unsigned depth);

    EvalRes eval_expr(Ref<Type> type, Ref<ast::Expr> expr, unsigned depth);
    EvalRes eval_list(Ref<Type> type, Ref<ast::InitList> list, unsigned depth);
    EvalRes eval_map(Ref<Type> type, Ref<ast::InitMap> map, unsigned depth);

    EvalRes
    eval_class_list(Ref<Class> cls, Ref<ast::InitList> list, unsigned depth);
    EvalRes eval_array_list(
        Ref<ArrayType> array_type, Ref<ast::InitList> list, unsigned depth);

    EvalRes
    eval_class_map(Ref<Class> cls, Ref<ast::InitMap> map, unsigned depth);
};

} // namespace ulam::sema
