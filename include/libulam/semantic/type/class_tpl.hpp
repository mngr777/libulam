#pragma once
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/class/base.hpp>
#include <libulam/semantic/type_tpl.hpp>
#include <libulam/str_pool.hpp>
#include <string>
#include <unordered_map>

namespace ulam::ast {
class ArgList;
class ClassDef;
} // namespace ulam::ast

namespace ulam {

class Class;
class Scope;

class ClassTpl : public TypeTpl, public ClassBase, public ScopeObject {
    friend Class;

public:
    ClassTpl(
        TypeIdGen& id_gen,
        UniqStrPool& str_pool,
        Ref<ast::ClassDef> node,
        Ref<Scope> scope);
    ~ClassTpl();

    str_id_t name_id() const;

    Ref<Type> type(
        Diag& diag,
        Ref<ast::ArgList> args_node,
        TypedValueList&& args) override;

private:
    Ptr<Class> inst(Ref<ast::ArgList> args_node, TypedValueList&& args);

    // TMP
    std::string type_args_str(const TypedValueList& args);

    UniqStrPool& _str_pool;
    Ref<ast::ClassDef> _node;
    std::unordered_map<std::string, Ptr<Class>> _classes;
};

} // namespace ulam
