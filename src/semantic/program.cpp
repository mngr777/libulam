#include <cassert>
#include <libulam/ast/nodes/root.hpp>
#include <libulam/semantic/program.hpp>
#include <libulam/semantic/scope.hpp>
#include <libulam/semantic/type/prim.hpp>
#include <libulam/str_pool.hpp>

namespace ulam {

Program::Program(
    Diag& diag, UniqStrPool& str_pool, UniqStrPool& text_pool, SrcMngr& sm):
    _diag{diag},
    _str_pool{str_pool},
    _text_pool{text_pool},
    _sm{sm},
    _type_id_gen{},
    _elements{},
    _builtins{this},
    _mangler{_text_pool} {}

Program::~Program() {}

Ref<Module> Program::module(const std::string_view name) {
    auto name_id = _str_pool.id(name);
    if (name_id == NoStrId)
        return {};
    return module(name_id);
}

Ref<Module> Program::module(str_id_t id) {
    auto it = _modules_by_name_id.find(id);
    if (it == _modules_by_name_id.end())
        return {};
    return it->second;
}

Ref<Module> Program::add_module(Ref<ast::ModuleDef> node) {
    assert(_modules_by_name_id.count(node->name_id()) == 0);
    auto mod = make<Module>(this, node);
    auto ref = ulam::ref(mod);
    _module_ptr.push_back(std::move(mod));
    _modules.push_back(ref);
    _modules_by_name_id[ref->name_id()] = ref;
    return ref;
}

} // namespace ulam
