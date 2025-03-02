#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/value/data.hpp>

namespace ulam {

class FunSet;

class BoundFunSet {
public:
    BoundFunSet(DataView self, Ref<FunSet> fset);

    DataView self() const { return _self; }

    Ref<FunSet> fset() const { return _fset; }

private:
    DataView _self;
    Ref<FunSet> _fset;
};

} // namespace ulam
