#include "./expr_res.hpp"
#include "./expr_flags.hpp"
#include <utility>

#ifdef DEBUG_EVAL
#    define ULAM_DEBUG
#    define ULAM_DEBUG_PREFIX "[compiler/ExprRes] "
#endif
#include "src/debug.hpp"

namespace {
using ExprRes = ulam::sema::ExprRes;
}

namespace exp {

std::string data(const ExprRes& res) {
    assert(res.has_data());
    assert(!res.data<std::string>().empty());
    auto data = res.data<std::string>();
    return data;
}

void set_data(ExprRes& res, std::string data) {
    assert(!data.empty());
    res.set_flags(exp::NoFlags);
    res.set_data<std::string>(std::move(data));
}

void set_data(ExprRes& res, const std::string_view data) {
    set_data(res, std::string{data});
}

void set_data(ExprRes& res, const char* data) {
    set_data(res, std::string{data});
}

void set_self(ExprRes& res) {
    assert(!res.has_data());
    set_data(res, "self");
    res.set_flag(exp::Self);
}

bool add_cast(ExprRes& res, bool expl) {
    // add cast unless it's an implicit cast on implicit cast result
    bool added = expl || !res.has_flag(ImplCast);
    res.uns_flag(ExplCast);
    res.uns_flag(ImplCast);
    if (added) {
        append(res, "cast");
        res.set_flag(expl ? ExplCast : ImplCast);
    }
    return added;
}

void add_member_access(ExprRes& res, const std::string& name, bool is_self) {
    append(res, name);
    append(res, ".");
    res.set_flag(is_self ? SelfMemberAccess : MemberAccess);
}

void add_member_access(
    ExprRes& res, const std::string_view name, bool is_self) {
    add_member_access(res, std::string{name}, is_self);
}

void add_member_access(ExprRes& res, const char* data, bool is_self) {
    add_member_access(res, std::string{data}, is_self);
}

void remove_member_access_op(ExprRes& res, bool remove_ident) {
    auto data = exp::data(res);
    auto size = data.size();

    // .
    assert(size > 2 && data[size - 2] == ' ' && data[size - 1] == '.');
    data = data.substr(0, size - 2);

    // member ident
    if (remove_ident) {
        auto pos = data.rfind(' ');
        assert(pos != std::string::npos);
        data = data.substr(0, pos);
    }

    set_data(res, data);
}

void add_array_access(
    ExprRes& res, const std::string& idx, bool before_member_access) {
    // ULAM quirk:
    // `a.b[c] -> `a b c [] .`, but
    // `/* self. */ b[c]` -> `self b . c []`

    if (!before_member_access) {
        append(res, idx);
        append(res, "[]");
        return;
    }

    remove_member_access_op(res);
    append(res, idx);
    append(res, "[]");
    append(res, ".");
}

void append(ExprRes& res, const std::string& data, const std::string& sep) {
    if (res.has_data()) {
        set_data(res, data_append(exp::data(res), data, sep));
    } else {
        set_data(res, data);
    }
}

std::string
data_append(std::string data1, std::string data2, const std::string& sep) {
    assert(!data1.empty() && !data2.empty());
    return data1 + sep + data2;
}

} // namespace exp
