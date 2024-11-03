#pragma once
#include <libulam/diag.hpp>
#include <libulam/src_mngr.hpp>

namespace ulam {

class Context {
public:
    Context(): _sm{}, _diag{_sm} {}

    SrcMngr& sm() { return _sm; }
    Diag& diag() { return _diag; }

private:
    SrcMngr _sm;
    Diag _diag;
};

}
