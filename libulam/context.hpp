#pragma once
#include <libulam/diag.hpp>
#include <libulam/options.hpp>
#include <libulam/src_man.hpp>

namespace ulam {

class Context {
public:
    Context(): _src_man{}, _diag{_src_man} {}

    Context(Context&&) = default;
    Context& operator=(Context&&) = default;

    SrcMan& src_man() { return _src_man; }
    Diag& diag() { return _diag; }

    Options options;

private:
    SrcMan _src_man;
    Diag _diag;
};

} // namespace ulam
