#include "src/parser/string.hpp"
#include <charconv>
#include <libulam/assert.hpp>
#include <libulam/context.hpp>
#include <libulam/diag.hpp>
#include <libulam/parser/options.hpp>
#include <libulam/parser/preproc.hpp>
#include <libulam/src.hpp>
#include <libulam/src_man.hpp>
#include <libulam/utils/file.hpp>
#include <utility>

#ifdef DEBUG_PREPROC
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[ulam::Preproc] "
#endif
#include "src/debug.hpp"

namespace ulam {

Preproc::Preproc(Context& ctx):
    _ctx{ctx},
    _path_resolver{ctx.options.include_paths},
    _version{DefaultVersion} {}

void Preproc::main_file(Path path) {
    ulam_assert(_stack.empty());
    auto src = _ctx.src_man().file(std::move(path));
    push(src);
}

void Preproc::main_string(std::string text, Path path) {
    ulam_assert(_stack.empty());
    auto src = _ctx.src_man().string(std::move(text), std::move(path));
    push(src);
}

void Preproc::add_string(std::string text, Path path) {
    _ctx.src_man().string(std::move(text), std::move(path));
}

Preproc& Preproc::operator>>(Token& token) {
    ulam_assert(!_stack.empty());
    while (true) {
        lex(token);
        switch (token.type()) {
        case tok::Ulam:
            preproc_ulam();
            break;
        case tok::Use:
            preproc_use();
            break;
        case tok::Load:
            if (!preproc_load())
                return *this;
            break;
        case tok::MlComment:
            preproc_meta(token);
        case tok::Comment:
            // ignore comments
            break;
        case tok::Self:
        case tok::Super:
            token.change_type(tok::Ident);
            return *this;
        case tok::SelfClass:
        case tok::SuperClass:
            token.change_type(tok::TypeIdent);
            return *this;
        case tok::IntT:
        case tok::UnsignedT:
        case tok::BoolT:
        case tok::UnaryT:
        case tok::BitsT:
        case tok::AtomT:
        case tok::VoidT:
        case tok::StringT:
            token.change_type(tok::BuiltinTypeIdent);
            return *this;
        case tok::__File:
        case tok::__FilePath:
        case tok::__Class:
        case tok::__ClassSignature:
        case tok::__ClassPretty:
        case tok::__ClassSimple:
        case tok::__ClassMangled:
        case tok::__Func:
            token.change_type(tok::String);
            return *this;
        case tok::__Line:
            token.change_type(tok::Number);
            return *this;
        case tok::Eof:
            _stack.pop();
            if (_stack.empty())
                return *this;
            break;
        default:
            return *this;
        }
    }
}

const Path& Preproc::current_path() const {
    ulam_assert(!_stack.empty());
    return _stack.top().first->path();
}

std::pair<std::string_view, SrcLoc> Preproc::move_last_meta_str() {
    std::string_view str;
    SrcLoc loc;
    return {std::exchange(_meta.str, str), std::exchange(_meta.loc, loc)};
}

void Preproc::push(Src* src) {
    ulam_assert(src);
    ulam_assert(src->content().start());
    _stack.emplace(
        src, Lex{_ctx.diag(), _ctx.src_man(), src->id(), src->content()});
}

Src& Preproc::src() {
    ulam_assert(!_stack.empty());
    return *_stack.top().first;
}

Lex& Preproc::lexer() {
    ulam_assert(!_stack.empty());
    return _stack.top().second;
}

void Preproc::lex(Token& token) {
    bool has_meta = _meta.flag;
    _meta.flag = false;

    lexer().lex(token);

    token.set_flag(tok::flags::HasMeta * has_meta);
}

template <typename... Ts>
bool Preproc::expect(Token& token, tok::Type type, Ts... stop) {
    lex(token);
    if (token.is(type))
        return true;
    while (true) {
        lex(token);
        if (token.in(stop...))
            break;
        _ctx.diag().error(token.loc_id(), token.size(), "Unexpected token");
    }
    return false;
}

void Preproc::preproc_ulam() {
    Token token;
    if (expect(token, tok::Number, tok::Semicol, tok::Eof)) {
        auto str = tok_str(token);
        version_t version{};
        auto [_, ec] = std::from_chars(str.begin(), str.end(), version);
        if (ec == std::errc{}) {
            _version = version;
        } else {
            _ctx.diag().warn(
                token.loc_id(), token.size(), "invalid version number");
        }
        expect(token, tok::Semicol, tok::Eof);
    }
}

void Preproc::preproc_use() {
    Token token;
    if (expect(token, tok::TypeIdent, tok::Semicol, tok::Eof)) {
        expect(token, tok::Semicol, tok::Eof);
    }
}

bool Preproc::preproc_load() {
    auto& src_man = _ctx.src_man();
    auto& diag = _ctx.diag();

    Token token;
    lexer().lex_path(token);

    // TODO: refactoring

    std::string_view path_str{tok_str(token)};
    assert(!empty(path_str));
    bool global = (path_str[0] == '<');
    // TODO: don't call loc_id() yet
    Path path{detail::parse_str(diag, token.loc_id(), path_str)};

    // hack for string sources
    auto src = src_man.src(path);
    if (dynamic_cast<StrSrc*>(src)) {
        bool ok = expect(token, tok::Semicol, tok::Eof);
        if (ok)
            push(src);
        return ok;
    }

    path = global ? _path_resolver.resolve(path)
                  : utils::find_file_rel(path, this->src().path());
    if (path.empty()) {
        _ctx.diag().fatal(token.loc_id(), token.size(), "file not found");
        return false;
    }
    src = src_man.src(path);
    if (!src)
        src = src_man.file(path);
    if (src) {
        bool ok = expect(token, tok::Semicol, tok::Eof);
        if (ok) {
            push(src);
            return true;
        }
    }
    return false;
}

void Preproc::preproc_meta(Token& token) {
    // NOTE: "/* */" regular comments are processed to possibly suggest changing
    // to "/** */"
    auto str = tok_str(token);
    ulam_assert(str.size() >= 4);
    if (str[2] != '*' && str.size() > 4)
        return; // ignoring regular comments for now

    _meta.str = str;
    _meta.flag = true;
}

const std::string_view Preproc::tok_str(const Token& token) {
    return _ctx.src_man().str_at(token.loc(), token.size());
}

} // namespace ulam
