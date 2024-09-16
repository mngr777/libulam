#pragma once
#include "libulam/types.hpp"
#include "src/source/line_buf.hpp"
#include "src/str_storage.hpp"
#include <filesystem>
#include <fstream>
#include <memory_resource>

namespace ulam {

class SourceManager;

class Source {
public:
    Source(
        SourceManager& sm,
        const SrcId id,
        std::string name,
        std::pmr::memory_resource* res);
    virtual ~Source() {};

    Source(const Source&) = delete;
    Source& operator=(const Source&) = delete;

    const SrcId id;
    const std::string name;

    std::istream& out() { return _out; }

    const SrcLocId loc_id();

protected:
    void init(std::istream* is);

private:
    SourceManager& _sm;
    StrStorage _lines;
    src::LineBuf _buf;
    std::istream _out;
};

class StrSource : public Source {
public:
    StrSource(
        SourceManager& sm,
        const SrcId id,
        std::string name,
        std::string text,
        std::pmr::memory_resource* res);
private:
    std::stringstream _is;
};

class FileSource : public Source {
public:
    FileSource(
        SourceManager& sm,
        const SrcId id,
        std::filesystem::path path,
        std::pmr::memory_resource* res);

private:
    std::ifstream _is;
};

} // namespace ulam
