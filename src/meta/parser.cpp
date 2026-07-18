#include "src/meta/parser.hpp"
#include "src/detail/string.hpp"
#include <libulam/assert.hpp>
#include <libulam/context.hpp>
#include <libulam/meta/field.hpp>

namespace ulam::meta {

Parser::Parser(Context& ctx, SrcLoc loc, std::string_view text):
    _ctx{ctx}, _loc{loc}, _text{text} {

    ulam_assert(!loc.empty());

    pos_t size = text.size();
    ulam_assert(text.size() >= 4); // /**/
    ulam_assert(text[0] == '/' && text[1] == '*');
    ulam_assert(text[size - 2] == '*' && text[size - 1] == '/');

    _is_meta = text[2] == '*' && size > 4; // starts with /**
    _pos = _is_meta ? 3 : 2;

    _linum = _loc.linum();
    _chr = _loc.chr();
}

char Parser::get(pos_diff_t off) const {
    ulam_assert(0 <= _pos + off && _pos + off < _text.size());
    return _text[_pos + off];
}

bool Parser::at(char ch) const { return get() == ch; }

bool Parser::is_eod() const { return _pos + 2 >= _text.size(); }

bool Parser::is_nl() const { return at('\n'); }

void Parser::advance() {
    ulam_assert(!is_eod());
    if (is_nl()) {
        ++_linum;
        _chr = 1;
    } else {
        ++_chr;
    }
    ++_pos;
}

loc_id_t Parser::loc_id() {
    return _ctx.src_man().loc_id({_loc.src_id(), _linum, _chr});
}

void Parser::skip_ws(bool skip_nl) {
    while (!is_eod() && (skip_nl || !is_nl()) && detail::is_whitespace(get()))
        advance();
}

void Parser::skip_line() {
    bool eol = false;
    while (!eol && !is_eod()) {
        eol = is_nl();
        advance();
    }
}

void Parser::read_desc() {
    skip_ws(true);
    if (is_eod() || at('\\'))
        return;

    pos_t start = _pos;
    pos_t end = _pos;
    while (!is_eod()) {
        if (is_nl()) {
            skip_line();
            skip_ws();
            if (at('\\'))
                break; // new line starts with '\'
            continue;
        }
        if (!detail::is_whitespace(get()))
            end = _pos + 1;
        advance();
    }
    _meta.set_desc(substr(start, end));
}

void Parser::parse_fields() {
    ulam_assert(is_eod() || at('\\'));
    while (at('\\')) {
        parse_field();
        skip_ws(true);
    }
}

void Parser::parse_field() {
    auto name = read_name();
    if (name.empty())
        return;

    auto field = find_field(name);
    Type type = field ? field->type() : Type::String;
    bool is_multi = field && field->has_flag(meta::Field::IsMulti);

    bool replaced = parse_value(std::move(name), type, is_multi);
    if (replaced)
        _ctx.diag().warn(loc_id(), _pos - 1, 1, "value replaced");
}

std::string Parser::read_name() {
    assert(at('\\'));
    advance();

    pos_t start = _pos;
    pos_t end = _pos;
    for (; !is_eod() && !detail::is_whitespace(get()); advance())
        end = _pos + 1;
    if (end == start) {
        _ctx.diag().warn(loc_id(), _pos - 1, 1, "empty meta field name");
        skip_line();
        skip_ws();
        return {};
    }
    return substr(start, end);
}

Meta Parser::parse() {
    read_desc();
    parse_fields();
    return std::move(_meta);
}

bool Parser::parse_value(std::string&& name, Type type, bool is_multi) {
    skip_ws();
    switch (type) {
    case Type::String:
        return parse_string(std::move(name), is_multi);
    }
    ulam_assert(false);
}

bool Parser::parse_string(std::string&& name, bool is_multi) {
    auto value = do_parse_string();
    if (is_multi) {
        _meta.append(std::move(name), Meta::List<std::string>{std::move(value)});
        return false;
    }
    return !_meta.set(std::move(name), std::move(value));
}

std::string Parser::do_parse_string() {
    ulam_assert(is_nl() || is_eod() || !detail::is_whitespace(get()));

    pos_t start = _pos;
    pos_t end = _pos;
    for (; !is_eod() && !is_nl(); advance()) {
        if (!detail::is_whitespace(get()))
            end = _pos + 1;
    }
    return substr(start, end);
}

std::string Parser::substr(pos_t start, pos_t end) const {
    ulam_assert(end >= start);
    return std::string{_text.substr(start, end - start)};
}

} // namespace ulam::meta
