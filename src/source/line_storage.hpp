#pragma once
#include "libulam/types.hpp"
#include "src/memory/notepad.hpp"
#include <memory_resource>
#include <shared_mutex>
#include <string_view>

namespace ulam::src {

class LineStorage {
public:
    static constexpr std::size_t NoOffset = -1;

    // TODO: store (very) short lines in struct?
    struct Rec {
        Rec(): offset(NoOffset) {}

        Rec(const std::size_t offset, const std::string_view line):
            offset(offset), line(line) {}

        bool empty() const { return line.empty(); }

        std::size_t offset;
        std::string_view line;
    };

    explicit LineStorage(std::pmr::memory_resource* res):
        _notepad(res), _lines(res ? res : std::pmr::get_default_resource()) {}

    const Rec get(const LineNum linum) const;

    void
    put(const LineNum linum,
        const std::size_t offset,
        const std::string_view line);

    bool has(const LineNum linum) const;

    const LineNum size() const;

private:
    void reserve(const LineNum linum);

    mem::Notepad _notepad;
    std::pmr::vector<Rec> _lines;
    mutable std::shared_mutex _mtx;
};

} // namespace ulam::src
