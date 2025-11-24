#include "./codegen.hpp"
#include "../out.hpp"
#include <cassert>
#include <sstream>

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

std::string Codegen::as_cond_str(
    ulam::Ref<ulam::ast::AsCond> as_cond, ulam::Ref<ulam::Type> type) {
    std::stringstream ss;
    auto strf = make_strf();
    auto name = _program->str_pool().get(as_cond->ident()->name_id());
    ss << name << " "
       << out::type_str(strf, type, false) << " as";
    return ss.str();
}

void Codegen::add_as_cond(
    ulam::Ref<ulam::ast::AsCond> as_cond, ulam::Ref<ulam::Type> type) {
    append(as_cond_str(as_cond, type));
    append("cond");
    set_next_prefix(as_cond_prefix_str(as_cond, type));
}

std::string Codegen::as_cond_prefix_str(
    ulam::Ref<ulam::ast::AsCond> as_cond, ulam::Ref<ulam::Type> type) {
    std::stringstream ss;
    auto strf = make_strf();
    auto name = _program->str_pool().get(as_cond->ident()->name_id());
    ss << " " << out::type_str(strf, type->ref_type()) << " " << name << "; ";
    return ss.str();
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
