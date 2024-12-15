#pragma once
#include <cstdint>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/type.hpp>
#include <variant>
#include <vector>

namespace ulam {
class Var;
}

namespace ulam::cls {

using layout_off_t = std::uint16_t;
constexpr layout_off_t NoLayoutOff = -1;

class Class;
class Layout;

class LayoutItem {
    friend Layout;

public:
    LayoutItem(Ref<Var> var): _obj{var} {}
    LayoutItem(Ref<Layout> layout): _obj{layout} {}

    bitsize_t bitsize() const;

private:
    using Variant = std::variant<Ref<Layout>, Ref<Var>>;

    Variant _obj;
    bitsize_t _offset{NoBitsize};
};

class Layout {
public:
    bitsize_t bitsize() const;

    void add(Ref<Var> var);
    void add(Ref<Layout> layout);

private:
    std::vector<LayoutItem> _items;
};

} // namespace ulam::cls
