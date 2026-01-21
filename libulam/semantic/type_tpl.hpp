#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/typed_value.hpp>

namespace ulam {

class TypeIdGen;

// TODO: remove?
class TypeTpl {
public:
    TypeTpl(TypeIdGen& id_gen): _id_gen{id_gen} {}

protected:
    TypeIdGen& id_gen() { return _id_gen; }

private:
    TypeIdGen& _id_gen;
};

} // namespace ulam
