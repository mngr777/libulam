#pragma once
#include <cassert>
#include <cstdint>
#include <list>
#include <stack>
#include <string>
#include <utility>
#include <variant>

namespace gen {

class ForContext {};

class WhileContext {};

class WhichContext {
public:
    WhichContext(const std::string& label_idx, const std::string& tmp_idx):
        _label_idx{label_idx},
        _tmp_type_name{"_SWITCHTYPEDEF" + tmp_idx},
        _tmp_var_name{"Uh_switchcond" + tmp_idx} {}

    WhichContext(WhichContext&&) = default;
    WhichContext& operator=(WhichContext&&) = default;

    unsigned cond_num() const { return _conds.size(); }
    void add_cond(std::string&& cond) { _conds.push_back(std::move(cond)); }
    std::string move_cond_str();

    bool has_non_default() const { return _non_default_num > 0; }
    unsigned non_default_num() const { return _non_default_num; }
    void inc_non_default_num() { ++_non_default_num; }

    bool case_has_breaks() const { return _case_has_breaks; }
    void set_case_has_breaks(bool has_breaks) {
        _case_has_breaks = has_breaks;
        _has_breaks = _has_breaks || has_breaks;
    }

    bool has_breaks() const { return _has_breaks; }

    const std::string& label_idx() const { return _label_idx; }

    const std::string& tmp_type_name() const { return _tmp_type_name; }
    const std::string& tmp_var_name() const { return _tmp_var_name; }

private:
    std::string _label_idx;
    std::string _tmp_type_name;
    std::string _tmp_var_name;
    std::list<std::string> _conds;
    bool _case_has_breaks{false};
    bool _has_breaks{false};
    unsigned _non_default_num{0};
};

class ContextStack {
public:
    using type_t = std::uint16_t;
    static constexpr type_t NoType = 0;

    class Raii {
        friend ContextStack;

    private:
        explicit Raii(ContextStack& stack): _stack{&stack} {}

    public:
        Raii(): _stack{} {}

        ~Raii() {
            if (_stack)
                _stack->pop();
        }

        Raii(Raii&& other) { operator=(std::move(other)); }

        Raii& operator=(Raii&& other) {
            std::swap(_stack, other._stack);
            return *this;
        }

    private:
        ContextStack* _stack;
    };

    ContextStack() {}

    ContextStack(const ContextStack&) = delete;
    ContextStack& operator=(const ContextStack&) = delete;

    std::size_t size() const { return _stack.size(); }
    bool empty() const { return _stack.empty(); }

    template <typename T> type_t top_is() const {
        return !empty() && std::holds_alternative<T>(_stack.top());
    }

    template <typename T> T& top() {
        assert(!empty());
        return std::get<T>(_stack.top());
    }

    template <typename T> void push(T&& ctx) { _stack.push(std::move(ctx)); }

    void pop() { _stack.pop(); }

    template <typename T> Raii raii(T&& ctx) {
        push(std::move(ctx));
        return Raii{*this};
    }

private:
    using Variant = std::variant<ForContext, WhileContext, WhichContext>;

    std::stack<Variant> _stack;
};

} // namespace gen
