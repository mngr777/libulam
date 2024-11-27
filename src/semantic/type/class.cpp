#include <cassert>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/ast/nodes/params.hpp>
#include <libulam/diag.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type/class.hpp>
#include <string>

namespace ulam {

// Class

Class::Class(Ref<Program> program, Ref<ClassTpl> tpl):
    BasicType(program->next_type_id()), _node{tpl->node()}, _tpl{tpl} {}

Class::Class(Ref<Program> program, ast::Ref<ast::ClassDef> node):
    BasicType(program->next_type_id()), _node{node}, _tpl{} {}

Class::~Class() {}

std::string Class::name() const {
    if (_tpl)
        return _tpl->name(); // TODO: params
    assert(_node);
    auto name_id = _node->name().str_id();
    return std::string{_program->ast()->ctx().str(name_id)};
}

Ref<Type> Class::type_member(str_id_t name_id) {
    auto sym = get(name_id);
    if (!sym && !sym->is<Type>())
        return {};
    return sym->get<Type>();
}

void Class::export_symbols(Scope* scope) {
    scope->import_symbols(_members, true /* skip typedefs */);
}

// ClassTpl

std::string ClassTpl::name() const {
    assert(_node);
    // TODO: refactoring
    auto name_id = _node->name().str_id();
    return std::string{program()->ast()->ctx().str(name_id)};
}

void ClassTpl::export_symbols(Scope* scope) { scope->import_symbols(_members); }

Ref<Type>
ClassTpl::type(ast::Ref<ast::ArgList> args_node, TypedValueList&& args) {
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
    auto cls = ulam::make<Class>(program(), this);
    auto params_node = _node->params();
    assert(params_node->child_num() > 0);
    assert(args_node->child_num() == args.size());
    unsigned n = 0;
    for (auto& arg : args) {
        // arg node
        auto arg_node = args_node->get(n);
        // too many arguments?
        if (n == params_node->child_num()) {
            diag().emit(
                diag::Error, arg_node->loc_id(), 1,
                "excessive class parameters");
            break;
        }
        // param node
        auto param_node = params_node->get(n);
        auto name_id = param_node->name().str_id();
        auto var = ulam::make<Var>(
            param_node->type_name(), ast::Ref<ast::VarDecl>{}, std::move(arg),
            Var::ClassParam);
        cls->set(name_id, std::move(var));
    }
    return cls;
}

std::string ClassTpl::type_args_str(const TypedValueList& args) {
    // this is a templorary implementation
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
            str += std::to_string(
                program()->ast()->ctx().str_id(rval->get<String>()));
        } else {
            assert(false);
        }
    }
    return str;
}

} // namespace ulam
