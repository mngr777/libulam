#include "libulam/semantic/type.hpp"
#include "libulam/semantic/type_tpl.hpp"
#include <cassert>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/ast/nodes/params.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type/class.hpp>
#include <string>

namespace ulam {

namespace {
Ptr<PersScope> make_class_scope(Ref<Scope> parent) {
    assert(parent && parent->is(Scope::Module));
    return make<PersScope>(parent, Scope::Class);
}

Ptr<PersScope> make_class_tpl_scope(Ref<Scope> parent) {
    assert(parent && parent->is(Scope::Module));
    return make<PersScope>(parent, Scope::ClassTpl);
}
} // namespace

// ClassBase

ClassBase::ClassBase(ast::Ref<ast::ClassDef> node, Ptr<PersScope>&& scope):
    _node{node}, _scope{std::move(scope)} {}

// Class

Class::Class(TypeIdGen& id_gen, Ref<ClassTpl> tpl):
    UserType{id_gen},
    ClassBase{tpl->node(), make_class_scope(tpl->scope()->parent())},
    _tpl{tpl} {}

Class::Class(TypeIdGen& id_gen, ast::Ref<ast::ClassDef> node, Ref<Scope> scope):
    UserType{id_gen}, ClassBase{node, make_class_scope(scope)}, _tpl{} {}

Class::~Class() {}

str_id_t Class::name_id() const { return node()->name().str_id(); }

// ClassTpl

ClassTpl::ClassTpl(
    TypeIdGen& id_gen, Ref<ast::ClassDef> node, Ref<Scope> scope):
    TypeTpl{id_gen}, ClassBase{node, make_class_tpl_scope(scope)} {}

ClassTpl::~ClassTpl() {}

str_id_t ClassTpl::name_id() const { return node()->name().str_id(); }

Ref<Type>
ClassTpl::type(Diag& diag, ast::Ref<ast::ArgList> args_node, TypedValueList&& args) {
    auto key = type_args_str(args);
    auto it = _types.find(key);
    if (it != _types.end())
        return ref(it->second);
    auto cls = inst(args_node, std::move(args));
    auto cls_ref = ref(cls);
    _types.emplace(key, std::move(cls));
    return cls_ref;
}

Ptr<Class>
ClassTpl::inst(ast::Ref<ast::ArgList> args_node, TypedValueList&& args) {
    auto cls = ulam::make<Class>(id_gen(), this);
    // copy members/scope objects
    auto scope_proxy = scope()->proxy();
    scope_proxy.reset();
    for (str_id_t name_id = scope_proxy.advance(); name_id != NoStrId;
         name_id = scope_proxy.advance()) {
        auto sym = scope_proxy.get(name_id);
        if (sym->is<UserType>()) {
            auto type = sym->get<UserType>();
            auto alias = type->as_alias();
            assert(alias);
            Ptr<UserType> copy =
                ulam::make<AliasType>(id_gen(), alias->node(), ref(cls));
            cls->scope()->set(name_id, ref(copy)); // add to class scope
            cls->set(name_id, std::move(copy));    // add class typedef

        } else if (sym->is<Var>()) {
            auto var = sym->get<Var>();
            Ptr<Var> copy{};
            if (var->is(Var::ClassParam)) {
                // class param
                assert(args.size() > 0);
                TypedValue value;
                std::swap(value, args.front());
                args.pop_front();
                copy = ulam::make<Var>(
                    var->type_node(), var->node(), std::move(value),
                    var->flags());
            } else {
                // class var
                copy = ulam::make<Var>(
                    var->type_node(), var->node(), Ref<Type>{}, var->flags());
            }
            cls->scope()->set(name_id, ref(copy)); // add to class scope
            cls->set(name_id, std::move(copy));    // add class var

        } else if (sym->is<Fun>()) {
            auto fun = sym->get<Fun>();
            auto copy = ulam::make<Fun>();
            for (auto& overload : fun->overloads())
                copy->add_overload(overload.ref()->node());
            cls->scope()->set(name_id, ref(copy)); // add to class scope
            cls->set(name_id, std::move(copy));    // add class fun

        } else {
            assert(false);
        }
        assert(cls->scope()->version() == scope_proxy.version()); // in sync?
    }
    assert(args.size() == 0); // all args consumed?
    return cls;
}

std::string ClassTpl::type_args_str(const TypedValueList& args) {
    // !! this is a temporary implementation
    std::string str;
    for (const auto& arg : args) {
        auto rval = arg.value().rvalue();
        assert(!rval->is_unknown());
        if (!str.empty())
            str += "_";
        if (rval->is<Integer>()) {
            str += std::to_string(rval->get<Integer>());
        } else if (rval->is<Unsigned>()) {
            str += std::to_string(rval->get<Unsigned>());
        } else if (rval->is<Bool>()) {
            str += (rval->get<Bool>() ? "t" : "f");
        } else if (rval->is<String>()) {
            // str += std::to_string(
            //     program()->ast()->ctx().str_id(rval->get<String>()));
            return rval->get<String>();
        } else {
            assert(false);
        }
    }
    return str;
}

} // namespace ulam
