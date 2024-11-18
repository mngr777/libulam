#pragma once
#include <libulam/ast.hpp>
#include <libulam/ast/nodes/module.hpp>
#include <libulam/sema/visitor.hpp>
#include <libulam/semantic/export_import.hpp>
#include <unordered_map>

namespace ulam::sema {

class ExportImport : public sema::RecVisitor {
public:
    explicit ExportImport(Sema& sema, ast::Ref<ast::Root> ast):
        RecVisitor{sema, ast} {}

    bool do_visit(ast::Ref<ast::TypeDef> node) override;
    bool do_visit(ast::Ref<ast::TypeSpec> node) override;

private:
    std::unordered_map<str_id_t, Export> _exports;
    std::unordered_map<str_id_t, Import> _maybe_imports;
    std::unordered_map<str_id_t, Import> _imports;
};

} // namespace ulam::sema
