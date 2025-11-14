#pragma once
#include <array>
#include <libulam/semantic/type/builtins.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/value/types.hpp>

class EvalTestContext {
public:
    static constexpr ulam::array_size_t EventWindowSize = 41;
    static constexpr ulam::array_size_t NeighborNum = EventWindowSize - 1;

    EvalTestContext(ulam::Builtins& builtins, ulam::LValue active_atom);
    EvalTestContext();

    EvalTestContext(EvalTestContext&&);
    EvalTestContext& operator=(EvalTestContext&&);

    bool empty() const;

    ulam::LValue active_atom();

    ulam::LValue neighbor(ulam::array_idx_t site) const;
    void set_neighbor(ulam::array_idx_t site, ulam::RValue&& rvalue);

private:
    ulam::Builtins* _builtins;
    ulam::LValue _active_atom;
    ulam::RValue _neighbors;
};
