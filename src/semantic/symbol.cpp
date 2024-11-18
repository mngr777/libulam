#include <libulam/semantic/symbol.hpp>

namespace ulam {

Symbol::~Symbol() {}

void SymbolTable::unset_placeholders() {
    for (auto it = _table.begin(); it != _table.end();) {
        if (it->second.is_placeholder()) {
            it = _table.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace ulam
