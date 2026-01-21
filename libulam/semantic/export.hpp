#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/module.hpp>
#include <libulam/str_pool.hpp>
#include <unordered_map>

namespace ulam {

class Export {
public:
    Export(Ref<Module> module, Module::Symbol* sym):
        _module{module}, _sym{sym} {}

    Ref<Module> module() const { return _module; }

    Module::Symbol* sym() const { return _sym; }

private:
    Ref<Module> _module;
    Module::Symbol* _sym;
};

class ExportTable {
public:
    bool has(str_id_t name_id) const;

    const Export* get(str_id_t name_id) const;

    // returns nullptr on success or conflicting export
    const Export* add(str_id_t name_id, Export exp);

private:
    std::unordered_map<str_id_t, Export> _table;
};

} // namespace ulam
