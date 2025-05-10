#include <libulam/sema/eval/cast.hpp>
#include <libulam/sema/eval/expr_visitor.hpp>
#include <libulam/sema/eval/funcall.hpp>
#include <libulam/sema/eval/init.hpp>
#include <libulam/sema/eval/visitor.hpp>
#include <libulam/semantic/typed_value.hpp>
#include <type_traits>

namespace ulam::sema {

std::pair<ArrayDimList, bool>
EvalInit::array_dims(unsigned num, Ref<ast::InitValue> init) {
    assert(num > 0);
    ArrayDimList dims;
    bool ok = true;

    if (!init->is<ast::InitList>()) {
        diag().error(init, "init value is not an array");
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
                diag().error(ref(other), "not an array");
                cur = {};
                ok = false;
            });
    }
    return {std::move(dims), ok};
}

ExprRes EvalInit::eval_init(Ref<VarBase> var, Ref<ast::InitValue> init) {
    assert(var->has_type());
    return eval_v(var, var->type(), init->get(), 1);
}

ExprRes EvalInit::eval_v(
    Ref<VarBase> var, Ref<Type> type, Variant& init, unsigned depth) {
    return init.accept(
        [&](Ptr<ast::Expr>& expr) {
            return eval_expr(var, type, ref(expr), depth);
        },
        [&](Ptr<ast::InitList>& list) {
            return eval_list(var, type, ref(list), depth);
        },
        [&](Ptr<ast::InitMap>& map) {
            return eval_map(var, type, ref(map), depth);
        });
}

ExprRes EvalInit::eval_expr(
    Ref<VarBase> var, Ref<Type> type, Ref<ast::Expr> expr, unsigned depth) {
    // eval
    auto ev = eval().expr_visitor(scope(), flags());
    auto res = expr->accept(*ev);
    if (!res)
        return res;
    return cast_expr_res(var, type, expr, std::move(res), depth);
}

ExprRes EvalInit::cast_expr_res(
    Ref<VarBase> var,
    Ref<Type> type,
    Ref<ast::Expr> expr,
    ExprRes&& res,
    unsigned depth) {
    assert(res);
    auto cast = eval().cast_helper(scope(), flags());
    return cast->cast(expr, type, std::move(res));
}

ExprRes EvalInit::eval_list(
    Ref<VarBase> var, Ref<Type> type, Ref<ast::InitList> list, unsigned depth) {
    if (type->is_class()) {
        return eval_class_list(var, type->as_class(), list, depth);
    } else if (type->is_array()) {
        return eval_array_list(var, type->as_array(), list, depth);
    } else {
        diag().error(
            list, "variable of scalar type cannot have initializer list");
        return {ExprError::NonScalarInit};
    }
}

ExprRes EvalInit::eval_map(
    Ref<VarBase> var, Ref<Type> type, Ref<ast::InitMap> map, unsigned depth) {
    if (type->is_class()) {
        return eval_class_map(var, type->as_class(), map, depth);
    } else {
        diag().error(
            map, "designated initializers are only supported for classes");
        return {ExprError::DesignatedInit};
    }
}

ExprRes EvalInit::eval_class_list(
    Ref<VarBase> var, Ref<Class> cls, Ref<ast::InitList> list, unsigned depth) {
    // eval args
    auto ev = eval().expr_visitor(scope(), flags());
    ExprResList args;
    for (unsigned n = 0; n < list->size(); ++n) {
        auto& item = list->get(n);
        if (!item.is<Ptr<ast::Expr>>()) {
            diag().error(
                list->child(n), "initializer list arguments are not supported");
            return {ExprError::InitListArgument};
        }
        auto& expr = item.get<Ptr<ast::Expr>>();
        auto expr_res = expr->accept(*ev);
        if (!expr_res)
            return expr_res;
        args.push_back(std::move(expr_res));
    }

    // call constructor or default-construct
    return construct_obj(var, cls, list, std::move(args));
}

ExprRes EvalInit::eval_array_list(
    Ref<VarBase> var,
    Ref<ArrayType> array_type,
    Ref<ast::InitList> list,
    unsigned depth) {
    auto item_type = array_type->item_type();
    auto size = array_type->array_size();

    // check size
    // for 1D arrays:
    // * if not enough items -- use last item in list (default construct for
    // empty list);
    // * if too many items -- ignore the rest. ?? only when array size is 0?
    bool autofill = !item_type->is_array();
    if (!autofill) {
        if (list->size() < size) {
            diag().error(list, "not enough items");
            return {ExprError::InitNotEnoughItems};
        } else if (size < list->size()) {
            diag().error(list, "too many items");
            return {ExprError::InitTooManyItems};
        }
    }

    // construct
    auto array = make_array(var, array_type);
    unsigned n = 0;
    ExprRes item;
    // fill with list items
    for (; n < std::min<unsigned>(list->size(), size); ++n) {
        // eval item
        item = eval_array_list_item(var, item_type, list->get(n), depth + 1);
        if (!item)
            return item;
        // assign to array item
        bool copy = (n + 1 == list->size() && size > list->size());
        if (!item.value().empty()) {
            auto item_copy = copy ? item.copy() : std::move(item);
            array = array_set(
                var, std::move(array), n, std::move(item_copy), false, depth);
        }
    }
    // fill rest with copies of the last value
    if (!item.value().empty()) {
        for (; n < size; ++n) {
            bool copy = (n + 1 < size);
            auto item_copy = copy ? item.copy() : std::move(item);
            array = array_set(
                var, std::move(array), n, std::move(item_copy), true, depth);
        }
    }
    return array;
}

ExprRes EvalInit::eval_class_map(
    Ref<VarBase> var, Ref<Class> cls, Ref<ast::InitMap> map, unsigned depth) {
    // construct
    auto obj = make_obj(var, cls);
    assert(map->size() > 0);

    for (auto key : map->keys()) {
        auto sym = cls->get(key);
        // not found?
        if (!sym) {
            auto message = std::string{"property `"} + std::string{str(key)} +
                           "' not found in " + cls->name();
            diag().error(map->child_by_key(key), std::move(message));
            return {ExprError::InitPropNotInClass};
        }
        // not a property?
        if (!sym->is<Prop>()) {
            auto message = cls->name() + "." + std::string{str(key)} +
                           " is not a property";
            diag().error(map->child_by_key(key), std::move(message));
            return {ExprError::InitNotProp};
        }

        // eval item
        auto prop = sym->get<Prop>();
        auto prop_res = eval_v(var, prop->type(), map->get(key), depth + 1);
        if (!prop_res)
            return prop_res;

        // assign
        obj = obj_set(var, std::move(obj), prop, std::move(prop_res), depth);
    }
    return obj;
}

ExprRes EvalInit::eval_array_list_item(
    Ref<VarBase> var, Ref<Type> type, Variant& item_v, unsigned depth) {
    return eval_v(var, type, item_v, depth);
}

ExprRes EvalInit::make_array(Ref<VarBase> var, Ref<ArrayType> array_type) {
    RValue rval = array_type->construct();
    rval.set_is_consteval(true);
    return {array_type, Value{std::move(rval)}};
}

ExprRes EvalInit::array_set(
    Ref<VarBase> var,
    ExprRes&& array,
    array_idx_t idx,
    ExprRes&& item,
    bool autofill,
    unsigned depth) {
    assert(array.type()->is_array());
    assert(idx < array.type()->as_array()->array_size());

    auto rval = array.move_value().move_rvalue();
    auto item_rval = item.move_value().move_rvalue();
    rval.set_is_consteval(rval.is_consteval() && item_rval.is_consteval());
    rval.array_access(idx, true).assign(std::move(item_rval));
    return {array.type(), Value{std::move(rval)}};
}

ExprRes EvalInit::make_obj(Ref<VarBase> var, Ref<Class> cls) {
    RValue rval = cls->construct();
    rval.set_is_consteval(true);
    return {cls, Value{std::move(rval)}};
}

ExprRes EvalInit::construct_obj(
    Ref<VarBase> var,
    Ref<Class> cls,
    Ref<ast::InitList> arg_list,
    ExprResList&& args) {
    if (args.size() == 0) {
        RValue rval = cls->construct();
        rval.set_is_consteval(true);
        return {cls, Value{std::move(rval)}};
    }
    auto funcall = eval().funcall_helper(scope(), flags());
    return funcall->construct(arg_list, cls, std::move(args));
}

ExprRes EvalInit::obj_set(
    Ref<VarBase> var,
    ExprRes&& obj,
    Ref<Prop> prop,
    ExprRes&& prop_res,
    unsigned depth) {
    assert(obj.type()->is_class());
    assert(prop_res.type()->is_same(prop->type()));

    auto rval = obj.move_value().move_rvalue();
    auto prop_rval = prop_res.move_value().move_rvalue();
    rval.set_is_consteval(rval.is_consteval() && prop_rval.is_consteval());
    rval.prop(prop).assign(std::move(prop_rval));
    return {obj.type(), Value{std::move(rval)}};
}

} // namespace ulam::sema
