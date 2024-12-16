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
    Src(src_id_t id, std::string&& name): _id{id}, _name{std::move(name)} {}
    virtual ~Src() {}

    Src(const Src&) = delete;
    Src& operator=(const Src&) = delete;

    virtual const mem::BufRef content() = 0;

    const mem::BufRef line(linum_t linum);

    const src_id_t id() const { return _id; }
    const std::string& name() const { return _name; }

private:
    src_id_t _id;
    std::string _name;
    std::vector<std::size_t> _line_off; // line offsets, starting from 2nd line
};

class FileSrc : public Src {
public:
    FileSrc(src_id_t id, std::filesystem::path path):
        Src{id, path.filename()}, _path{path} {}

    const mem::BufRef content() override;

private:
    std::filesystem::path _path;
    std::optional<mem::Buf> _buf;
};

class StrSrc : public Src {
public:
    StrSrc(src_id_t id, std::string text, std::string name):
        Src{id, std::move(name)}, _text{std::move(text)} {}

    const mem::BufRef content() override {
        return {_text.c_str(), _text.size() + 1};
    }

private:
    std::string _text;
};

} // namespace ulam
