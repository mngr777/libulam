#pragma once
#include <cassert>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/value/types.hpp>
#include <limits>

namespace ulam::detail {

// TODO: refactoring, optimizations

static_assert(sizeof(Integer) == sizeof(Unsigned));

template <typename T> constexpr T min() {
    return std::numeric_limits<T>::min();
}

template <typename T> constexpr T max() {
    return std::numeric_limits<T>::max();
}

constexpr int sign(Integer value) {
    return (value == 0) ? 0 : (value < 0 ? -1 : 1);
}

constexpr Unsigned abs(Integer value) { return value < 0 ? -value : value; }

constexpr Unsigned log2(Unsigned value) {
    bitsize_t size = 0;
    do {
        ++size;
        value >>= 1;
    } while (value);
    return size;
}

constexpr std::uint32_t ones32(bitsize_t n) {
    return (n == 32) ? -1 : ((std::uint32_t)1 << n) - 1;
}

constexpr std::uint64_t ones64(bitsize_t n) {
    return (n == 64) ? -1 : ((std::uint64_t)1 << n) - 1;
}

constexpr Unsigned ones(bitsize_t n) {
    if constexpr (sizeof(Unsigned) == 8)
        return ones64(n);
    static_assert(sizeof(Unsigned) == 4);
    return ones32(n);
}

constexpr Unsigned count_ones(Unsigned value) {
    return __builtin_popcount(value);
}

constexpr bitsize_t bitsize(Unsigned value) { return log2(value); }

constexpr bitsize_t unary_unsigned_bitsize(bitsize_t bitsize) {
    return log2(bitsize);
}

constexpr bitsize_t bitsize(Integer value) { return bitsize(abs(value)) + 1; }

constexpr Unsigned unsigned_max(bitsize_t size) {
    assert(0 < size && size <= sizeof(Unsigned) * 8);
    return (size == (sizeof(Unsigned) * 8)) ? max<Unsigned>() : (1 << size) - 1;
}

constexpr Integer integer_min(bitsize_t size) {
    assert(1 < size && size <= sizeof(Integer) * 8);
    return -unsigned_max(size - 1) - 1;
}

constexpr Integer integer_max(bitsize_t size) {
    assert(1 < size && size <= sizeof(Integer) * 8);
    return unsigned_max(size - 1);
}

constexpr Unsigned truncate(Unsigned value, bitsize_t size) {
    return std::min(value, unsigned_max(size));
}

constexpr Integer truncate(Integer value, bitsize_t size) {
    return (value < 0) ? std::max(value, integer_min(size))
                       : std::min(value, integer_max(size));
}

constexpr std::pair<Unsigned, Unsigned>
safe_sum(Unsigned left, Unsigned right) {
    constexpr auto Max = max<Unsigned>();
    auto max_right = Max - right;
    if (Max > max_right)
        return {Max, right - max_right};
    return {left + right, 0};
}

constexpr std::pair<Integer, Integer> safe_sum(Integer left, Integer right) {
    if (left < 0 && right < 0) {
        constexpr auto Min = min<Integer>();
        auto min_right = Min - left;
        if (right < min_right)
            return {Min, right - min_right};
        return {left + right, 0};

    } else if (left > 0 && right > 0) {
        constexpr auto Max = max<Integer>();
        auto max_right = Max - left;
        if (right > max_right)
            return {Max, right - max_right};
        return {left + right, 0};

    } else {
        return {left + right, 0};
    }
}

constexpr std::pair<Unsigned, Unsigned>
safe_diff(Unsigned left, Unsigned right) {
    if (left < right)
        return {0, right - left};
    return {left - right, 0};
}

constexpr std::pair<Integer, Integer> safe_diff(Integer left, Integer right) {
    if (right == min<Integer>()) {
        constexpr auto Max = max<Integer>();
        if (left < 0)
            return {Max + left + 1, 0};
        return {Max, -1 - left};
    }
    auto [diff, overflow] = safe_sum(left, -right);
    return {diff, -overflow};
}

// max save 2nd factor
constexpr Unsigned safe_prod_max(Unsigned left) {
    return std::numeric_limits<Unsigned>::max() / left;
}

// {max safe 2nd factor, max(Unsigned) - left * right}
constexpr std::pair<Unsigned, Unsigned> safe_prod_max_rem(Unsigned left) {
    constexpr auto max = std::numeric_limits<Unsigned>::max();
    auto max_right = max / left;
    return {max_right, max - left * max_right};
}

// {min(max(Unsigned), left * right), truncated?}
constexpr std::pair<Unsigned, bool> safe_prod(Unsigned left, Unsigned right) {
    auto max_right = safe_prod_max(left);
    auto is_truncated = (right > max_right);
    return {
        (is_truncated) ? std::numeric_limits<Unsigned>::max() : left * right,
        is_truncated};
}

// {min(max(Integer), max(min(Integer), left * right)), truncated?}
constexpr std::pair<Integer, bool> safe_prod(Integer left, Integer right) {
    constexpr auto Min = std::numeric_limits<Integer>::min();
    constexpr auto Max = std::numeric_limits<Integer>::max();
    auto left_sign = sign(left);
    auto right_sign = sign(right);
    switch (left_sign * right_sign) {
    case 0:
        return {0, false};
    case 1: {
        if (left == Min || right == Min)
            return {Max, true};
        auto right_threshold = Max / left;
        if (right_threshold < 0) {
            assert(left < 0 && right < 0);
            bool is_truncated = (right < right_threshold);
            return {is_truncated ? Max : left * right, is_truncated};
        } else {
            assert(right_threshold > 0);
            assert(left > 0 && right > 0);
            bool is_truncated = (right > right_threshold);
            return {is_truncated ? Max : left * right, is_truncated};
        }
    }
    case -1: {
        if (left == -1) {
            assert(right > 0);
            return {-right, false};
        }
        auto right_threshold = Min / left;
        if (right_threshold < 0) {
            assert(left > 0 && right < 0);
            bool is_truncated = (right < right_threshold);
            return {is_truncated ? Min : left * right, is_truncated};
        } else {
            assert(left < 0 && right > 0);
            assert(right_threshold > 0);
            bool is_truncated = (right > right_threshold);
            return {is_truncated ? Min : left * right, is_truncated};
        }
    }
    default:
        assert(false);
    }
}

template <typename T> T safe_quot(T left, T right) {
    return right == 0 ? 0 : left / right;
}

template <typename T> T safe_rem(T left, T right) {
    return right == 0 ? left : left % right;
}

} // namespace ulam::detail
