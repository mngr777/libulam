#include <iostream>
#include <libulam/semantic/detail/integer.hpp>
#include <libulam/semantic/value/bits.hpp>
#include <libulam/semantic/value/types.hpp>
#include <limits>
#include <vector>

struct IntegerCase {
    ulam::Integer val;
    ulam::bitsize_t size;
    std::vector<ulam::bitsize_t> offs;
};

static const ulam::bitsize_t IntSize = sizeof(ulam::Integer) * 8;
static const ulam::Integer IntMin = std::numeric_limits<ulam::Integer>::min();
static const ulam::Integer IntMax = std::numeric_limits<ulam::Integer>::max();

static IntegerCase integer_cases[] = {
    {1, 32, {0, 1, 31, 32, 63, 64}},
    {-1, 32, {0, 1, 31, 32, 63, 64}},
    {-1, 2, {0, 1, 31, 32, 63, 64}},
    {-1000,
     ulam::detail::bitsize((ulam::Integer)-1000),
     {0, 1, 31, 32, 63, 64}},
    {IntMin, IntSize, {0, 1, 31, 32, 63, 64}},
    {IntMax, IntSize, {0, 1, 31, 32, 63, 64}},
};

int main() {
    for (auto test : integer_cases) {
        for (auto off : test.offs) {
            ulam::Bits bits{128};
            ulam::Datum datum1 =
                ulam::detail::integer_to_datum(test.val, test.size);
            bits.write(off, test.size, datum1);
            ulam::Datum datum2 = bits.read(off, test.size);
            assert(datum2 == datum1);
            ulam::Integer val =
                ulam::detail::integer_from_datum(datum2, test.size);
            if (val != test.val) {
                std::cerr << val << " != " << test.val << "\n";
                return -1;
            }
        }
    }
    return 0;
}
