#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/type.hpp>
#include <variant>
#include <vector>

namespace ulam {
class Var;
}

namespace ulam::cls {

class Layout;

class LayoutItem {
    friend Layout;

public:
    LayoutItem(Ref<Var> obj): _obj{obj} {}

    bitsize_t bitsize() const;

private:
    using Variant = std::variant<Layout*, Ref<Var>>;

    bool has_offset() const { return _offset != NoBitsize; }
    bitsize_t offset() const { return _offset; }
    void set_offset(bitsize_t offset) { _offset = offset; }

    Variant _obj;
    bitsize_t _offset{NoBitsize};
};

class Layout {
public:
    bitsize_t bitsize() const;

    void add(Ref<Var> var);

private:
    std::vector<LayoutItem> _items;
};

} // namespace ulam::cls
