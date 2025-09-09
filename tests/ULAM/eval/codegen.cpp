#include "./codegen.hpp"
#include "../out.hpp"
#include <cassert>

Stringifier Codegen::make_strf() { return Stringifier{_program}; }

void Codegen::block_open(bool nospace) {
    auto prefix = move_next_prefix();
    append("{", nospace);
    if (!prefix.empty())
        append(std::move(prefix), true);
}

void Codegen::block_close(bool nospace) { append("}", nospace); }

void Codegen::append(std::string data, bool nospace) {
    if (!nospace && !_code.empty())
        _code += " ";
    if (!_next_prefix.empty())
        _code += move_next_prefix();
    _code += std::move(data);
}

void Codegen::add_as_cond(
    ulam::Ref<ulam::ast::AsCond> as_cond, ulam::Ref<ulam::Type> type) {
    std::string name{_program->str_pool().get(as_cond->ident()->name_id())};
    auto strf = make_strf();
    append(name);
    append(out::type_str(strf, type, false));
    append("as");
    append("cond");
    set_next_prefix(
        " " + out::type_str(strf, type->ref_type()) + " " + name + "; ");
}

bool Codegen::has_next_prefix() const {
    return !_next_prefix.empty();
}

void Codegen::set_next_prefix(std::string prefix) {
    assert(!prefix.empty());
    assert(_next_prefix.empty());
    _next_prefix = prefix;
}

std::string Codegen::move_next_prefix() {
    std::string prefix;
    std::swap(prefix, _next_prefix);
    return prefix;
}
