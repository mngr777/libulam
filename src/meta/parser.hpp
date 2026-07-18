#pragma once
#include <libulam/diag.hpp>
#include <libulam/memory/buf.hpp>
#include <libulam/meta.hpp>
#include <libulam/src_loc.hpp>

namespace ulam {
class Context;
}

namespace ulam::meta {

class Parser {
public:
    Parser(Context& ctx, SrcLoc loc, std::string_view text);

    Meta parse();

private:
    using pos_t = std::size_t;
    using pos_diff_t = std::ptrdiff_t;

    static constexpr pos_t NoPos = -1;

    char get(pos_diff_t off = 0) const;
    bool at(char ch) const;

    bool is_eod() const;
    bool is_nl() const;

    void advance();

    loc_id_t loc_id();

    void skip_ws(bool skip_nl = false);
    void skip_line();

    void read_desc();
    void parse_fields();
    void parse_field();

    std::string read_name();

    bool parse_value(std::string&& name, Type type, bool is_multi);
    bool parse_string(std::string&& name, bool is_multi);

    std::string do_parse_string();

    std::string substr(pos_t start, pos_t end) const;

    Context& _ctx;
    SrcLoc _loc;
    const std::string_view _text;
    pos_t _pos;
    bool _is_meta;
    Meta _meta;

    linum_t _linum;
    chr_t _chr;
};

} // namespace ulam::meta
