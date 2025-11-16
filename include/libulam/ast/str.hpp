#pragma once
#include "libulam/src_loc.hpp"
#include <libulam/src_man.hpp>
#include <libulam/str_pool.hpp>

namespace ulam::ast {

// TODO: add length
// NOTE: loc_id/length are for error reporting purposes and do not have to match
// the string, e.g. function name string for operator can be "operator+=" while
// loc_id/length point to "operator +=" (with a space) in program text

class Str {
public:
    Str(str_id_t str_id, loc_id_t loc_id): _str_id{str_id}, _loc_id{loc_id} {}
    Str(): Str{NoStrId, NoLocId} {}

    bool empty() const { return _str_id == NoStrId; }
    bool has_loc() const { return _loc_id != NoLocId; }

    str_id_t str_id() const { return _str_id; }
    loc_id_t loc_id() const { return _loc_id; }

private:
    str_id_t _str_id;
    loc_id_t _loc_id;
};

} // namespace ulam::ast
