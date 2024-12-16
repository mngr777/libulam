#pragma once
#include <libulam/memory/notepad.hpp>
#include <unordered_map>
#include <vector>

namespace ulam {

using str_id_t = std::uint32_t;
constexpr str_id_t NoStrId = -1;

class StrPoolBase {
public:
    StrPoolBase() {}
    virtual ~StrPoolBase() {}

    StrPoolBase(const StrPoolBase&) = delete;
    StrPoolBase& operator=(const StrPoolBase&) = delete;

    virtual str_id_t put(const std::string_view str, bool copy = true) = 0;
    const std::string_view get(str_id_t id) const;

protected:
    using PairT = std::pair<str_id_t, const std::string_view>;

    PairT store(const std::string_view str, bool copy);

    mem::Notepad _notepad;
    std::vector<std::string_view> _index;
};

class UniqStrPool : public StrPoolBase {
public:
    bool has(const std::string_view str) const;
    str_id_t id(const std::string_view str) const;

    str_id_t put(const std::string_view str, bool copy = true);

private:
    std::unordered_map<std::string_view, str_id_t> _map;
};

class StrPool : public StrPoolBase {
public:
    virtual str_id_t put(const std::string_view str, bool copy = true);
};

} // namespace ulam
