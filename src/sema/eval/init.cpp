#include "libulam/sema/eval/flags.hpp"
#include <libulam/sema/eval/cast.hpp>
#include <libulam/sema/eval/env.hpp>
#include <libulam/sema/eval/expr_visitor.hpp>
#include <libulam/sema/eval/funcall.hpp>
#include <libulam/sema/eval/init.hpp>
#include <libulam/semantic/typed_value.hpp>
#include <type_traits>

// Where passing default_lval is actually needed (see t41167-...):
// * init map is applied to object with a default value previously defined
//   elsewhere,  e.g. if A is defined `element A { B b = {.foo=1, .bar=2} }`,
//   then in `A a = {.b = {.foo = 3}}` init map for `A.b` must be merged
//   with default value;
// * similar case, when default object values are inside an array:
//   `element A { B[] bs={{.foo=1, .bar=2}, <...>} }`
// * if empty array `{}` is provided by init map, default value should be used
//   instead (see t41206), `QBar bar = {.r = {}, .sp = false}`

namespace ulam::sema {

bool EvalInit::init_var(Ref<Var> var, Ref<ast::InitValue> init, bool in_expr) {
    if (!init) {
        var_init_default(var, in_expr);
        return true;
    }
    auto res = eval_init(var, init);
    if (res) {
        var_init_expr(var, std::move(res), in_expr);
        return true;
    }
    return false;
}

bool EvalInit::init_var_with(Ref<Var> var, ExprRes&& init) {
    assert(init);
    assert(var->has_type());
    init = env().cast(var->node(), var->type(), std::move(init));
    if (!init)
        return false;
    var_init_expr(var, std::move(init), false);
    return true;
}

bool EvalInit::init_prop(Ref<Prop> prop, Ref<ast::InitValue> init) {
    if (!init) {
        prop_init_default(prop);
        return true;
    }
    auto res = eval_init(prop, init);
    if (res) {
        prop_init_expr(prop, std::move(res));
        return true;
    }
    return false;
}

ExprRes EvalInit::eval_init(Ref<VarBase> var, Ref<ast::InitValue> init) {
    assert(var->has_type());
    return eval_v(var, var->type(), LValue{}, init->get(), 1);
}

void EvalInit::var_init_expr(Ref<Var> var, ExprRes&& init, bool in_expr) {
    assert(init);
    var_init_common(var, in_expr);
    var->set_value(init.move_value());
}

void EvalInit::var_init_default(Ref<Var> var, bool in_expr) {
    var_init_common(var, in_expr);
    auto type = var->type();
    auto rval = (!has_flag(evl::NoExec) || var->is_const())
                    ? type->construct()
                    : type->construct_ph();
    var->set_value(Value{std::move(rval)});
}

void EvalInit::var_init_common(Ref<Var> var, bool in_expr) {
    assert(var && var->has_type());
    assert(var->value().empty());
}

void EvalInit::prop_init_expr(Ref<Prop> prop, ExprRes&& init) {
    assert(init);
    prop_init_common(prop);
    prop->set_default_value(init.move_value().move_rvalue());
}

void EvalInit::prop_init_default(Ref<Prop> prop) {
    prop_init_common(prop);
    prop->set_default_value(prop->type()->construct());
}

void EvalInit::prop_init_common(Ref<Prop> prop) {
    assert(prop && prop->has_type());
    assert(!prop->has_default_value());
}

ExprRes EvalInit::eval_v(
    Ref<VarBase> var,
    Ref<Type> type,
    LValue default_lval,
    Variant& init,
    unsigned depth) {
    return init.accept(
        [&](Ptr<ast::Expr>& expr) {
            return eval_expr(var, type, ref(expr), depth);
        },
        [&](Ptr<ast::InitList>& list) {
            return eval_list(var, type, default_lval, ref(list), depth);
        },
        [&](Ptr<ast::InitMap>& map) {
            return eval_map(var, type, default_lval, ref(map), depth);
        });
}

ExprRes EvalInit::eval_expr(
    Ref<VarBase> var, Ref<Type> type, Ref<ast::Expr> expr, unsigned depth) {
    auto res = env().eval_expr(expr);
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
    return env().cast(expr, type, std::move(res));
}

ExprRes EvalInit::eval_list(
    Ref<VarBase> var,
    Ref<Type> type,
    LValue default_lval,
    Ref<ast::InitList> list,
    unsigned depth) {

    if (type->is_class()) {
        // NOTE: constructor call, so no default rvalue passed
        return eval_class_list(var, type->as_class(), list, depth);
    } else if (type->is_array()) {
        // NOTE: default rvalue passed when items are objects, so may need to be
        // merged
        return eval_array_list(
            var, type->as_array(), default_lval, list, depth);
    } else {
        diag().error(
            list, "variable of scalar type cannot have initializer list");
        return {ExprError::NonScalarInit};
    }
}

ExprRes EvalInit::eval_map(
    Ref<VarBase> var,
    Ref<Type> type,
    LValue default_lval,
    Ref<ast::InitMap> map,
    unsigned depth) {

    if (type->is_class()) {
        // NOTE: default rvalue is passed to be merged with map data
        return eval_class_map(var, type->as_class(), default_lval, map, depth);
    } else {
        diag().error(
            map, "designated initializers are only supported for classes");
        return {ExprError::DesignatedInit};
    }
}

ExprRes EvalInit::eval_class_list(
    Ref<VarBase> var, Ref<Class> cls, Ref<ast::InitList> list, unsigned depth) {
    ExprResList args;
    for (unsigned n = 0; n < list->size(); ++n) {
        auto& item = list->get(n);
        if (!item.is<Ptr<ast::Expr>>()) {
            diag().error(
                list->child(n), "initializer list arguments are not supported");
            return {ExprError::InitListArgument};
        }
        auto& expr = item.get<Ptr<ast::Expr>>();
        auto expr_res = env().eval_expr(expr.get());
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
    LValue default_lval,
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
    bool use_default = (list->empty() && !default_lval.empty());
    auto array =
        make_array(var, array_type, use_default ? default_lval : LValue{});

    unsigned n = 0;
    ExprRes item;
    // fill with list items
    for (; n < std::min<unsigned>(list->size(), size); ++n) {
        // eval item
        LValue item_default_lval;
        if (!default_lval.empty()) {
            item_default_lval = default_lval.array_access(n, true);
        }
        item = eval_array_list_item(
            var, item_type, item_default_lval, list->get(n), depth + 1);
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
    Ref<VarBase> var,
    Ref<Class> cls,
    LValue default_lval,
    Ref<ast::InitMap> map,
    unsigned depth) {
    // construct
    auto obj = make_obj(var, cls, default_lval);
    assert(map->size() > 0);

    for (auto key : map->keys()) {
        auto sym = cls->get(key);
        // not found?
        if (!sym) {
            auto message = std::string{"property `"} + std::string{str(key)} +
                           "' not found in " + std::string{cls->name()};
            diag().error(map->child_by_key(key), std::move(message));
            return {ExprError::InitPropNotInClass};
        }
        // not a property?
        if (!sym->is<Prop>()) {
            auto message = std::string{cls->name()} + "." +
                           std::string{str(key)} + " is not a property";
            diag().error(map->child_by_key(key), std::move(message));
            return {ExprError::InitNotProp};
        }

        // eval item
        auto prop = sym->get<Prop>();
        auto type = prop->type();
        LValue default_lval;
        if (type->is_object() || type->is_array())
            default_lval = obj.value().prop(prop).lvalue();
        auto prop_res =
            eval_v(var, type, default_lval, map->get(key), depth + 1);
        if (!prop_res)
            return prop_res;

        // assign
        obj = obj_set(var, std::move(obj), prop, std::move(prop_res), depth);
    }
    return obj;
}

ExprRes EvalInit::eval_array_list_item(
    Ref<VarBase> var,
    Ref<Type> type,
    LValue default_lval,
    Variant& item_v,
    unsigned depth) {
    return eval_v(var, type, default_lval, item_v, depth);
}

ExprRes EvalInit::make_array(
    Ref<VarBase> var, Ref<ArrayType> array_type, LValue default_lval) {
    auto rval = (!default_lval.empty()) ? default_lval.rvalue()
                                        : array_type->construct();
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

ExprRes
EvalInit::make_obj(Ref<VarBase> var, Ref<Class> cls, LValue default_lval) {
    RValue rval =
        (!default_lval.empty()) ? default_lval.rvalue() : cls->construct();
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
    return env().construct(arg_list, cls, std::move(args));
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
