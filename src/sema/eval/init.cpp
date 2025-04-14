#include "libulam/semantic/value.hpp"
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

ExprRes EvalInit::eval_init(Ref<Type> type, Ref<ast::InitValue> init) {
    return eval_v(type, init->get(), 1);
}

ExprRes EvalInit::eval_v(Ref<Type> type, Variant& init, unsigned depth) {
    return init.accept(
        [&](Ptr<ast::Expr>& expr) { return eval_expr(type, ref(expr), depth); },
        [&](Ptr<ast::InitList>& list) {
            return eval_list(type, ref(list), depth);
        },
        [&](Ptr<ast::InitMap>& map) {
            return eval_map(type, ref(map), depth);
        });
}

ExprRes
EvalInit::eval_expr(Ref<Type> type, Ref<ast::Expr> expr, unsigned depth) {
    // eval
    auto ev = eval().expr_visitor(scope(), flags());
    auto res = expr->accept(*ev);

    // cast
    if (res) {
        auto cast = eval().cast_helper(scope(), flags());
        res = cast->cast(expr, type, std::move(res));
    }
    return res;
}

ExprRes
EvalInit::eval_list(Ref<Type> type, Ref<ast::InitList> list, unsigned depth) {
    if (type->is_class()) {
        return eval_class_list(type->as_class(), list, depth);
    } else if (type->is_array()) {
        return eval_array_list(type->as_array(), list, depth);
    } else {
        diag().error(
            list, "variable of scalar type cannot have initializer list");
        return {ExprError::NonScalarInit};
    }
}

ExprRes
EvalInit::eval_map(Ref<Type> type, Ref<ast::InitMap> map, unsigned depth) {
    if (type->is_class()) {
        return eval_class_map(type->as_class(), map, depth);
    } else {
        diag().error(
            map, "designated initializers are only supported for classes");
        return {ExprError::DesignatedInit};
    }
}

ExprRes EvalInit::eval_class_list(
    Ref<Class> cls, Ref<ast::InitList> list, unsigned depth) {
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

    // call constructor
    auto funcall = eval().funcall_helper(scope(), flags());
    return funcall->construct(list, cls, std::move(args));
}

ExprRes EvalInit::eval_array_list(
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
            diag().error(list, "not enough items");
            return {ExprError::InitNotEnoughItems};
        } else if (size < list->size()) {
            diag().error(list, "too many items");
            return {ExprError::InitTooManyItems};
        }
    }

    // construct
    auto array = make_array(array_type);
    unsigned n = 0;
    ExprRes item;
    // fill with list items
    for (; n < std::min<unsigned>(list->size(), size); ++n) {
        // eval item
        item = eval_v(item_type, list->get(n), depth + 1);
        if (!item)
            return item;
        // assign to array item
        bool copy = (n + 1 == list->size() && size > list->size());
        if (!item.value().empty()) {
            array = array_set(
                std::move(array), n, copy ? item.copy() : std::move(item));
        }
    }
    // fill rest with copies of the last value
    if (!item.value().empty()) {
        for (; n < size; ++n) {
            bool copy = (n + 1 < size);
            array = array_set(
                std::move(array), n, copy ? item.copy() : std::move(item));
        }
    }
    return array;
}

ExprRes EvalInit::eval_class_map(
    Ref<Class> cls, Ref<ast::InitMap> map, unsigned depth) {
    // construct
    auto obj = make_obj(cls);
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
        auto prop_res = eval_v(prop->type(), map->get(key), 1);
        if (!prop_res)
            return prop_res;

        // assign
        obj = obj_set(std::move(obj), prop, std::move(prop_res));
    }
    return obj;
}

ExprRes EvalInit::make_array(Ref<ArrayType> array_type) {
    RValue rval = array_type->construct();
    rval.set_is_consteval(true);
    return {array_type, Value{std::move(rval)}};
}

ExprRes EvalInit::array_set(ExprRes&& array, array_idx_t idx, ExprRes&& item) {
    assert(array.type()->is_array());
    assert(idx < array.type()->as_array()->array_size());

    auto rval = array.move_value().move_rvalue();
    auto item_rval = item.move_value().move_rvalue();
    rval.set_is_consteval(rval.is_consteval() && item_rval.is_consteval());
    rval.array_access(idx).assign(std::move(item_rval));
    return {array.type(), Value{std::move(rval)}};
}

ExprRes EvalInit::make_obj(Ref<Class> cls) {
    RValue rval = cls->construct();
    rval.set_is_consteval(true);
    return {cls, Value{std::move(rval)}};
}

ExprRes EvalInit::obj_set(ExprRes&& obj, Ref<Prop> prop, ExprRes&& prop_res) {
    assert(obj.type()->is_class());
    assert(prop_res.type() == prop->type());

    auto rval = obj.move_value().move_rvalue();
    auto prop_rval = prop_res.move_value().move_rvalue();
    rval.set_is_consteval(rval.is_consteval() && prop_rval.is_consteval());
    rval.prop(prop).assign(std::move(prop_rval));
    return {obj.type(), Value{std::move(rval)}};
}

} // namespace ulam::sema
