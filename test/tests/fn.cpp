#include <catch2/catch_test_macros.hpp>
#include <lbyte/stx/fn.hpp>

using namespace lbyte::stx;

static int add_ints(int a, int b) { return a + b; }
static int multiply_ints(int a, int b) { return a * b; }
static int double_int(int val) { return val * 2; }
static int get_forty_two() { return 42; }
static double double_value(double x) { return x * 2.0; }
static int sum_four(int a, int b, int c, int d) { return a + b + c + d; }
static int* return_pointer() { static int val = 100; return &val; }

TEST_CASE("Fn: caller_t structure")
{
    SECTION("caller_t has correct function pointer type")
    {
        using sig_t = caller_t<int(double, char)>;
        static_assert(std::is_same_v<sig_t::fn_t, int(*)(double, char)>);
    }

    SECTION("caller_t stores function pointer")
    {
        caller_t<int(int, int)> c{add_ints};
        REQUIRE(c.fn != nullptr);
        REQUIRE(static_cast<bool>(c));
    }

    SECTION("caller_t invokes function pointer")
    {
        caller_t<int(int, int)> c{multiply_ints};
        REQUIRE(c(3, 4) == 12);
    }

    SECTION("caller_t with void return")
    {
        int result = 0;
        caller_t<int(int)> c{double_int};
        result = c(21);
        REQUIRE(result == 42);
    }

    SECTION("caller_t converts to bool correctly")
    {
        caller_t<int()> c{nullptr};
        REQUIRE(!c);

        caller_t<int()> valid_c{get_forty_two};
        REQUIRE(valid_c);
    }
}

TEST_CASE("Fn: caller factory function")
{
    SECTION("caller creates caller_t from function pointer")
    {
        auto c = caller<int(int, int)>(add_ints);
        REQUIRE(c(5, 3) == 8);
    }

    SECTION("caller creates caller_t from va_s")
    {
        va_s addr{reinterpret_cast<uptr>(multiply_ints)};
        auto c = caller<int(int, int)>(addr);
        REQUIRE(c(10, 3) == 30);
    }

    SECTION("caller creates caller_t from uintptr_t")
    {
        uptr addr = reinterpret_cast<uptr>(double_value);
        auto c = caller<double(double)>(addr);
        REQUIRE(c(3.5) == 7.0);
    }
}

TEST_CASE("Fn: Complex signatures")
{
    SECTION("caller with many parameters")
    {
        auto c = caller<int(int, int, int, int)>(sum_four);
        REQUIRE(c(1, 2, 3, 4) == 10);
    }

    SECTION("caller with no parameters")
    {
        auto c = caller<int()>(get_forty_two);
        REQUIRE(c() == 42);
    }

    SECTION("caller with pointer return type")
    {
        auto c = caller<int*()>(return_pointer);
        int* result = c();
        REQUIRE(*result == 100);
    }
}
