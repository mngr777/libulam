#include <libulam/sema/eval/cast.hpp>
#include <libulam/sema/eval/expr_visitor.hpp>
#include <libulam/sema/eval/funcall.hpp>
#include <libulam/sema/eval/init.hpp>
#include <libulam/sema/eval/visitor.hpp>
#include <libulam/semantic/typed_value.hpp>

namespace ulam::sema {

std::pair<ArrayDimList, bool>
EvalInit::array_dims(unsigned num, Ref<ast::InitValue> init) {
    assert(num > 0);
    ArrayDimList dims;
    bool ok = true;

    if (!init->is<ast::InitList>()) {
        _diag.error(init, "init value is not an array");
        return {std::move(dims), false};
    }

    // get array dimensions
    auto cur = init->get<ast::InitList>();
    while (cur) {
        dims.push_back(cur->size());
        if (dims.size() == num)
            break; // done

        auto& first = cur->get(0);
        first.accept(
            [&](Ptr<ast::InitList>& sublist) { cur = ref(sublist); },
            [&](auto&& other) {
                _diag.error(ref(other), "not an array");
                cur = {};
                ok = false;
            });
    }
    return {std::move(dims), ok};
}

EvalInit::EvalRes EvalInit::eval(Ref<Type> type, Ref<ast::InitValue> init) {
    return eval_v(type, init->get(), 1);
}

EvalInit::EvalRes
EvalInit::eval_v(Ref<Type> type, Variant& init, unsigned depth) {
    return init.accept(
        [&](Ptr<ast::Expr>& expr) { return eval_expr(type, ref(expr), depth); },
        [&](Ptr<ast::InitList>& list) {
            return eval_list(type, ref(list), depth);
        },
        [&](Ptr<ast::InitMap>& map) {
            return eval_map(type, ref(map), depth);
        });
}

EvalInit::EvalRes
EvalInit::eval_expr(Ref<Type> type, Ref<ast::Expr> expr, unsigned depth) {
    // eval
    auto ev = _eval.expr_visitor(_scope);
    auto res = expr->accept(*ev);

    // cast
    if (res) {
        auto cast = _eval.cast_helper(_scope);
        res = cast->cast(expr, type, res.move_typed_value());
    }
    if (!res)
        return {Value{RValue{}}, false};
    return {res.move_value(), true};
}

EvalInit::EvalRes
EvalInit::eval_list(Ref<Type> type, Ref<ast::InitList> list, unsigned depth) {
    if (type->is_class()) {
        return eval_class_list(type->as_class(), list, depth);
    } else if (type->is_array()) {
        return eval_array_list(type->as_array(), list, depth);
    } else {
        _diag.error(
            list, "variable of scalar type cannot have initializer list");
        return {Value{RValue{}}, false};
    }
}

EvalInit::EvalRes
EvalInit::eval_map(Ref<Type> type, Ref<ast::InitMap> map, unsigned depth) {
    return {Value{RValue{}}, false}; // TODO
}

EvalInit::EvalRes EvalInit::eval_class_list(
    Ref<Class> cls, Ref<ast::InitList> list, unsigned depth) {
    auto rval = cls->construct();
    if (list->size() == 0) {
        rval.set_is_consteval(true);
        return {Value{std::move(rval)}, true};
    }

    // eval args
    auto ev = _eval.expr_visitor(_scope);
    TypedValueList args;
    for (unsigned n = 0; n < list->size(); ++n) {
        auto& item = list->get(n);
        if (!item.is<Ptr<ast::Expr>>()) {
            _diag.error(
                list->child(n), "initializer list arguments are not supported");
            return {Value{RValue{}}, false};
        }
        auto& expr = item.get<Ptr<ast::Expr>>();
        auto expr_res = expr->accept(*ev);
        if (!expr_res)
            return {Value{RValue{}}, false};
        args.push_back(expr_res.move_typed_value());
    }

    // call constructor
    auto funcall = _eval.funcall_helper(_scope);
    auto funcall_res = funcall->funcall(
        list, cls->constructors(), rval.self(), std::move(args));
    if (!funcall_res)
        return {Value{RValue{}}, false};
    return {Value{std::move(rval)}, true};
}

EvalInit::EvalRes EvalInit::eval_array_list(
    Ref<ArrayType> array_type, Ref<ast::InitList> list, unsigned depth) {
    auto item_type = array_type->item_type();
    auto size = array_type->array_size();

    // check size
    // for 1D arrays:
    // * if not enough items -- use last item in list (default construct for
    // empty list);
    // * if too many items -- ignore the rest. ?? only when array size is 0?
    bool autofill = (depth == 1) && !item_type->is_array();
    if (!autofill) {
        if (list->size() < size) {
            _diag.error(list, "not enough items");
            return {Value{RValue{}}, false};
        } else if (size < list->size()) {
            _diag.error(list, "too many items");
            return {Value{RValue{}}, false};
        }
    }

    // construct
    RValue rval = array_type->construct();
    bool is_consteval = true;

    unsigned n = 0;
    Value item_val{RValue{}};
    bool ok{};
    // fill with list items
    for (; n < std::min<unsigned>(list->size(), size); ++n) {
        // eval item
        std::tie(item_val, ok) = eval_v(item_type, list->get(n), depth + 1);
        if (!ok)
            return {Value{RValue{}}, ok};
        // all consteval?
        is_consteval =
            is_consteval && !item_val.empty() && item_val.is_consteval();
        // assign to array item
        bool copy = (n + 1 == list->size() && size > list->size());
        if (!item_val.empty()) {
            rval.array_access(n).assign(
                copy ? item_val.copy_rvalue() : item_val.move_rvalue());
        }
    }
    // fill rest with copies of the last value
    if (!item_val.empty()) {
        for (; n < size; ++n) {
            bool copy = (n + 1 < size);
            rval.array_access(n).assign(
                copy ? item_val.copy_rvalue() : item_val.move_rvalue());
        }
    }
    rval.set_is_consteval(is_consteval);
    return {Value{std::move(rval)}, true};
}

} // namespace ulam::sema
