#pragma once
#include "libulam/src_loc.hpp"
#include "libulam/memory/buf.hpp"
#include <filesystem>
#include <optional>

namespace ulam {

class Src {
public:
    Src(src_id_t id): _id{id} {}
    virtual ~Src() {}

    Src(const Src&) = delete;
    Src& operator=(const Src&) = delete;

    virtual const mem::BufRef content() = 0;

    const src_id_t id() { return _id; }

private:
    src_id_t _id;
};

class FileSrc : public Src {
public:
    FileSrc(src_id_t id, std::filesystem::path path): Src{id}, _path{path} {}

    const mem::BufRef content() override;

private:
    std::filesystem::path _path;
    std::optional<mem::Buf> _buf;
};

class StrSrc : public Src {
public:
    StrSrc(src_id_t id, std::string text): Src{id}, _text{std::move(text)} {}

    const mem::BufRef content() override {
        return {_text.c_str(), _text.size() + 1};
    }

private:
    std::string _text;
};

} // namespace ulam
