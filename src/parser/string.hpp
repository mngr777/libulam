#pragma once
#include <libulam/semantic/value.hpp>
#include <libulam/src_mngr.hpp>
#include <string_view>

namespace ulam {
class Diag;
}

namespace ulam::detail {

String parse_str(Diag& diag, loc_id_t loc_id, const std::string_view str);

}
