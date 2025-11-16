#pragma once
#include <libulam/diag.hpp>
#include <libulam/options.hpp>
#include <libulam/src_man.hpp>

namespace ulam {

class Context {
public:
    Context(): _sm{}, _diag{_sm} {}

    Context(Context&&) = default;
    Context& operator=(Context&&) = default;

    SrcMan& sm() { return _sm; }
    Diag& diag() { return _diag; }

    Options options;

private:
    SrcMan _sm;
    Diag _diag;
};

} // namespace ulam
