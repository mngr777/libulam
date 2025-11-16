#pragma once
#include <libulam/semantic/number.hpp>
#include <libulam/src_man.hpp>
#include <string_view>

namespace ulam {
class Diag;
} // namespace ulam

namespace ulam::detail {

Number parse_num_str(Diag& diag, loc_id_t loc_id, const std::string_view str);
Number parse_char_str(Diag& diag, loc_id_t loc_id, const std::string_view str);

} // namespace ulam::detail
