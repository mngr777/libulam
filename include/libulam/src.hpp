#pragma once
#include "libulam/memory/buf.hpp"
#include "libulam/src_loc.hpp"
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace ulam {

class Src {
public:
    Src(src_id_t id, std::filesystem::path path): _id{id}, _path{std::move(path)} {}
    virtual ~Src() {}

    Src(const Src&) = delete;
    Src& operator=(const Src&) = delete;

    virtual const mem::BufRef content() = 0;

    const mem::BufRef line(linum_t linum);

    const src_id_t id() const { return _id; }
    const std::filesystem::path& path() const { return _path; }

private:
    src_id_t _id;
    std::filesystem::path _path;
    std::vector<std::size_t> _line_off; // line offsets, starting from 2nd line
};

class FileSrc : public Src {
public:
    FileSrc(src_id_t id, std::filesystem::path path):
        Src{id, path}, _path{path} {}

    const mem::BufRef content() override;

private:
    std::filesystem::path _path;
    std::optional<mem::Buf> _buf;
};

class StrSrc : public Src {
public:
    StrSrc(src_id_t id, std::string text, std::filesystem::path path):
        Src{id, std::move(path)}, _text{std::move(text)} {}

    const mem::BufRef content() override {
        return {_text.c_str(), _text.size() + 1};
    }

private:
    std::string _text;
};

} // namespace ulam
