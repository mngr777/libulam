#pragma once
#include <libulam/memory/notepad.hpp>
#include <unordered_map>
#include <vector>

namespace ulam {

using str_id_t = std::uint16_t;
constexpr str_id_t NoStrId = -1;

class StrPoolBase {
public:
    StrPoolBase() {}
    virtual ~StrPoolBase() {}

    StrPoolBase(const StrPoolBase&) = delete;
    StrPoolBase& operator=(const StrPoolBase&) = delete;

    virtual str_id_t put(const std::string_view str, bool copy = true) = 0;
    virtual const std::string_view get(str_id_t id) const;

protected:
    using PairT = std::pair<str_id_t, const std::string_view>;

    PairT store(const std::string_view str, bool copy);

    mem::Notepad _notepad;
    std::vector<std::string_view> _index;
};

class UniqStrPool : public StrPoolBase {
public:
    explicit UniqStrPool(UniqStrPool* parent = {});

    bool has(const std::string_view str) const;
    str_id_t id(const std::string_view str) const;

    str_id_t put(const std::string_view str, bool copy = true) override;
    const std::string_view get(str_id_t id) const override;

private:
    UniqStrPool* _parent;
    str_id_t _offset{0};
    bool _is_locked{false}; // marks parent pool as locked (assert check only)
    std::unordered_map<std::string_view, str_id_t> _map;
};

class StrPool : public StrPoolBase {
public:
    str_id_t put(const std::string_view str, bool copy = true) override;
};

} // namespace ulam
