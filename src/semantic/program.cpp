#include <cassert>
#include <libulam/ast/nodes/root.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type/prim.hpp>
#include <libulam/str_pool.hpp>
#include <utility>

namespace ulam {

Program::Program(
    Diag& diag, UniqStrPool& str_pool, UniqStrPool& text_pool, SrcMngr& sm):
    _diag{diag},
    _str_pool{str_pool},
    _text_pool{text_pool},
    _sm{sm},
    _type_id_gen{},
    _elements{_options.class_options},
    _builtins{this},
    _mangler{_text_pool} {}

Program::~Program() {}

const ClassOptions& Program::class_options() const {
    return _options.class_options;
}

ClassOptions Program::set_class_options(ClassOptions options) {
    std::swap(_options.class_options, options);
    return options;
}

const ScopeOptions& Program::scope_options() const {
    return _options.scope_options;
}

ScopeOptions Program::set_scope_options(ScopeOptions options) {
    std::swap(_options.scope_options, options);
    return options;
}

Ref<Module> Program::module(const std::string_view name) {
    auto name_id = _str_pool.id(name);
    return (name_id != NoStrId) ? module(name_id) : Ref<Module>{};
}

Ref<Module> Program::module(str_id_t id) {
    auto it = _modules_by_name_id.find(id);
    return (it != _modules_by_name_id.end()) ? it->second : Ref<Module>{};
}

Ref<Module> Program::add_module(Ref<ast::ModuleDef> node) {
    assert(_modules_by_name_id.count(node->name_id()) == 0);
    auto mod = make<Module>(this, node);
    auto ref = ulam::ref(mod);
    _module_ptrs.push_back(std::move(mod));
    _modules.push_back(ref);
    _modules_by_name_id[ref->name_id()] = ref;
    return ref;
}

const Export* Program::add_export(str_id_t name_id, Export exp) {
    return _exports.add(name_id, std::move(exp));
}

} // namespace ulam
