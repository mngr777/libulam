#pragma once
#include <libulam/sema/expr_res.hpp>
#include <string>
#include <string_view>

namespace exp {

std::string data(const ulam::sema::ExprRes& res);
void set_data(ulam::sema::ExprRes& res, std::string data);
void set_data(ulam::sema::ExprRes& res, const std::string_view data);
void set_data(ulam::sema::ExprRes& res, const char* data);

void set_self(ulam::sema::ExprRes& res);

bool add_cast(ulam::sema::ExprRes& res, bool expl = false);

void add_member_access(
    ulam::sema::ExprRes& res, const std::string& name, bool is_self);
void add_member_access(
    ulam::sema::ExprRes& res, const std::string_view name, bool is_self);
void add_member_access(
    ulam::sema::ExprRes& res, const char* data, bool is_self);

void remove_member_access_op(ulam::sema::ExprRes& res);

void add_array_access(
    ulam::sema::ExprRes& res,
    const std::string& idx,
    bool before_member_access);

void append(
    ulam::sema::ExprRes& res,
    const std::string& data,
    const std::string& sep = " ");

std::string
data_append(std::string data1, std::string data2, const std::string& sep = " ");

template <typename T, typename... Ts>
std::string data_combine(T data, Ts... rest) {
    if constexpr (sizeof...(rest) == 0) {
        return std::string{data};
    } else {
        return data_append(std::string{data}, data_combine(rest...));
    }
}

} // namespace exp
