#pragma once
#include <libulam/diag.hpp>
#include <libulam/options.hpp>
#include <libulam/src_mngr.hpp>

namespace ulam {

class Context {
public:
    Context(): _sm{}, _diag{_sm} {}

    Context(Context&&) = default;
    Context& operator=(Context&&) = default;

    SrcMngr& sm() { return _sm; }
    Diag& diag() { return _diag; }

    Options options;

private:
    SrcMngr _sm;
    Diag _diag;
};

} // namespace ulam
