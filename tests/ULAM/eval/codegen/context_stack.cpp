#include "./context_stack.hpp"

namespace gen {

std::string WhichContext::move_cond_str() {
    std::string str;
    for (const auto& cond : _conds)
        str += (str.empty() ? "" : " ") + cond;
    for (unsigned i = 1; i < _conds.size(); ++i)
        str += " ||";
    _conds.clear();
    return str;
}

} // namespace gen
