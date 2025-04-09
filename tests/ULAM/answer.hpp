#pragma once
#include <map>
#include <ostream>
#include <string>

class Answer {
public:
    const std::string& class_name() const { return _class_name; }
    void set_class_name(std::string name) { _class_name = std::move(name); }

    const std::string& test_fun() const { return _test_fun; }
    void set_test_fun(std::string text) { _test_fun = std::move(text); }

    bool has_type_def(const std::string& alias) const;
    std::string type_def(const std::string& alias) const;
    void add_type_def(std::string alias, std::string text);

    bool has_prop(const std::string& name) const;
    std::string prop(const std::string& name) const;
    void add_prop(std::string name, std::string text);

    const auto& props() const { return _props; }
    const auto& type_defs() const { return _type_defs; }

private:
    std::string _class_name;
    std::string _test_fun;
    std::map<std::string, std::string> _props;
    std::map<std::string, std::string> _type_defs;
};

using AnswerMap = std::map<std::string, Answer>;

Answer parse_answer(const std::string_view text);

void compare_answer_maps(
    const AnswerMap& true_answers, const AnswerMap& answers);
void compare_answers(const Answer& truth, const Answer& answer);

std::ostream& operator<<(std::ostream& os, const Answer& answer);
