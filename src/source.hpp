#pragma once
#include "libulam/types.hpp"
#include "src/source/line_buf.hpp"
#include "src/source/line_storage.hpp"
#include <filesystem>
#include <string_view>
#include <fstream>
#include <memory_resource>
#include <optional>

namespace ulam {

class SourceManager;
class Source;

class SourceStream : public std::istream {
public:
    SourceStream(Source& src, src::LineBuf& line_buf):
        std::istream(&line_buf), _src(src), _line_buf(line_buf) {}

    const SrcLocId last_loc_id();
    const std::string_view substr() const;

private:
    Source& _src;
    src::LineBuf& _line_buf;
};

class Source {
    friend SourceStream;

public:
    Source(
        SourceManager& sm,
        const SrcId id,
        std::string name,
        std::pmr::memory_resource* res);
    virtual ~Source() {}

    Source(const Source&) = delete;
    Source& operator=(const Source&) = delete;

    SourceStream& stream();

    const SrcId id() const { return _id; }
    const std::string& name() const { return _name; }

protected:
    virtual std::istream& input() = 0;

private:
    struct Output {
        Output(Source& src):
            line_buf(src._lines, src.input(), src._res),
            stream(src, line_buf) {}

        src::LineBuf line_buf;
        SourceStream stream;
    };

    const SrcLocId loc_id(const LineNum linum, const CharNum chr);

    SourceManager& _sm;
    std::pmr::memory_resource* _res;
    src::LineStorage _lines;
    SrcId _id;
    std::string _name;
    std::optional<Output> _out;
};

class StrSource : public Source {
public:
    StrSource(
        SourceManager& sm,
        const SrcId id,
        std::string name,
        std::string text,
        std::pmr::memory_resource* res);

protected:
    std::istream& input() override { return _is; }

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

protected:
    std::istream& input() override { return _is; }

private:
    std::ifstream _is;
};

} // namespace ulam
