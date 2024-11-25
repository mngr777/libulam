#include "libulam/semantic/value.hpp"
#include <libulam/ast/nodes/module.hpp>
#include <libulam/ast/nodes/params.hpp>
#include <libulam/diag.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type/class.hpp>
#include <libulam/semantic/program.hpp>
#include <string>

namespace ulam {

// Class

Class::Class(type_id_t id, Ref<ClassTpl> tpl):
    BasicType(id), _node{tpl->node()}, _tpl{tpl} {}

Class::Class(type_id_t id, ast::Ref<ast::ClassDef> node):
    BasicType(id), _node{node}, _tpl{} {}

Class::~Class() {}

void Class::export_symbols(Scope* scope) { scope->import_symbols(_members); }

// ClassTpl

void ClassTpl::export_symbols(Scope* scope) { scope->import_symbols(_members); }

Ref<Type> ClassTpl::type(ast::Ref<ast::ArgList> args, ValueList& values) {
    auto key = param_values_str(values);
    {
        auto it = _types.find(key);
        if (it != _types.end())
            return ref(it->second);
    }
    auto cls = make<Class>(program()->next_type_id(), this);
    auto params = _node->params();
    assert(params->child_num() > 0);
    assert(args->child_num() == values.size());
    // unsigned n = 0;
    // for (auto& val : values) {
    //     // arg node
    //     auto arg = args->get(n);
    //     // too many arguments?
    //     if (n == params->child_num()) {
    //         diag().emit(diag::Error, arg->loc_id(), 1, "excessive class parameters");
    //         break;
    //     }
    //     // param node
    //     auto param = params->get(n);
    //     auto name_id = param->name().str_id();
    //     // auto var = make<Var>();
    // }
    return {};
}

std::string ClassTpl::param_values_str(const ValueList& values) {
    std::string str;
    for (const auto& val : values) {
        auto rval = val.rvalue();
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
            str += std::to_string(program()->ast()->ctx().str_id(rval->get<String>()));
        } else {
            assert(false);
        }
    }
    return str;
}

} // namespace ulam
