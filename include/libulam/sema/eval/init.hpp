#pragma once
#include <libulam/ast/nodes/expr.hpp>
#include <libulam/ast/nodes/var_decl.hpp>
#include <libulam/detail/variant.hpp>
#include <libulam/diag.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/value/types.hpp>
#include <libulam/str_pool.hpp>

namespace ulam::sema {

class EvalVisitor;

class EvalInit {
public:
    using EvalRes = std::pair<Value, bool>;

    EvalInit(
        EvalVisitor& eval, Diag& diag, UniqStrPool& str_pool, Ref<Scope> scope):
        _eval{eval}, _diag{diag}, _str_pool{str_pool}, _scope{scope} {}

    // {dimensions, is_array}
    std::pair<ArrayDimList, bool>
    array_dims(unsigned num, Ref<ast::InitValue> init);

    virtual EvalRes eval(Ref<Type> type, Ref<ast::InitValue> init);

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

    EvalVisitor& _eval;
    Diag& _diag;
    UniqStrPool& _str_pool;
    Ref<Scope> _scope;
};

} // namespace ulam::sema
