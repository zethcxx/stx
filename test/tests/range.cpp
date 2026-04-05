#include <catch2/catch_test_macros.hpp>
#include <lbyte/stx/range.hpp>

#include <vector>

using namespace lbyte;
#include <array>

TEST_CASE("Range: Enums")
{
    SECTION("range_dir values")
    {
        REQUIRE(stx::range_dir::Forward  == stx::range_dir{0});
        REQUIRE(stx::range_dir::Backward == stx::range_dir{1});
    }

    SECTION("range_mode values")
    {
        REQUIRE(stx::range_mode::Inclusive == stx::range_mode{0});
        REQUIRE(stx::range_mode::Exclusive == stx::range_mode{1});
    }
}

TEST_CASE("Range: Basic forward exclusive range")
{
    SECTION("range from 0 to n with default step")
    {
        std::vector<int> values;

        for (auto i : stx::range(5, stx::range_dir::Forward))
        {
            values.push_back(i);
        }

        REQUIRE(values == std::vector<int>{0, 1, 2, 3, 4});
    }

    SECTION("range from 0 to n with Backward direction")
    {
        std::vector<int> values;

        for (auto i : stx::range(5, stx::range_dir::Backward))
        {
            values.push_back(i);
        }

        REQUIRE(values == std::vector<int>{4, 3, 2, 1, 0});
    }
}

TEST_CASE("Range: Forward range with inclusive mode")
{
    SECTION("irange from 0 to n")
    {
        std::vector<int> values;

        for (auto i : stx::irange(5, stx::range_dir::Forward))
        {
            values.push_back(i);
        }

        REQUIRE(values == std::vector<int>{0, 1, 2, 3, 4, 5});
    }
}

TEST_CASE("Range: Range with custom from/to")
{
    SECTION("Forward exclusive from to")
    {
        std::vector<int> values;

        for (auto i : stx::range(3, 8, stx::range_dir::Forward))
        {
            values.push_back(i);
        }

        REQUIRE(values == std::vector<int>{3, 4, 5, 6, 7});
    }

    SECTION("Backward exclusive from to")
    {
        std::vector<int> values;

        for (auto i : stx::range(8, 3, stx::range_dir::Backward))
        {
            values.push_back(i);
        }

        REQUIRE(values == std::vector<int>{7, 6, 5, 4, 3});
    }

    SECTION("Forward inclusive from to")
    {
        std::vector<int> values;

        for (auto i : stx::irange(3, 7, stx::details::base_type_t<int>{1}, stx::range_dir::Forward))
        {
            values.push_back(i);
        }

        REQUIRE(values == std::vector<int>{3, 4, 5, 6, 7});
    }

    SECTION("Backward inclusive from to")
    {
        std::vector<int> values;

        for (auto i : stx::irange(7, 3, stx::details::base_type_t<int>{1}, stx::range_dir::Backward))
        {
            values.push_back(i);
        }

        REQUIRE(values == std::vector<int>{7, 6, 5, 4, 3});
    }
}

TEST_CASE("Range: Range with custom step")
{
    SECTION("Forward with step 2")
    {
        std::vector<int> values;

        for (auto i : stx::range(0, 10, 2, stx::range_dir::Forward))
        {
            values.push_back(i);
        }

        REQUIRE(values == std::vector<int>{0, 2, 4, 6, 8});
    }

    SECTION("Backward with step 3")
    {
        std::vector<int> values;

        for (auto i : stx::range(30, 0, 3, stx::range_dir::Backward))
        {
            values.push_back(i);
        }

        REQUIRE(values == std::vector<int>{27, 24, 21, 18, 15, 12, 9, 6, 3});
    }
}

TEST_CASE("Range: Range with strong types")
{
    SECTION("range with off_s")
    {
        std::vector<stx::off_s::value_type> values;

        for (auto i : stx::range(stx::off_s{0}, stx::off_s{5}, stx::range_dir::Forward))
        {
            values.push_back(i.get());
        }

        REQUIRE(values == std::vector<stx::usize>{0, 1, 2, 3, 4});
    }

    SECTION("range with rva_s")
    {
        std::vector<stx::rva_s::value_type> values;

        for (auto i : stx::range(stx::rva_s{10}, stx::rva_s{15}, stx::range_dir::Forward))
        {
            values.push_back(i.get());
        }

        REQUIRE(values == std::vector<stx::u32>{10, 11, 12, 13, 14});
    }
}

TEST_CASE("Range: Can use with std::array")
{
    std::array<int, 5> arr{1, 2, 3, 4, 5};
    int sum = 0;

    for (auto i : stx::range(5, stx::range_dir::Forward))
    {
        sum += arr[i];
    }

    REQUIRE(sum == 15);
}

TEST_CASE("Range: Empty ranges")
{
    SECTION("Forward range where from > to")
    {
        std::vector<int> values;

        for (auto i : stx::range(10, 5, stx::range_dir::Forward))
        {
            values.push_back(i);
        }

        REQUIRE(values.empty());
    }

    SECTION("Backward range where from < to")
    {
        std::vector<int> values;

        for (auto i : stx::range(5, 10, stx::range_dir::Backward))
        {
            values.push_back(i);
        }

        REQUIRE(values.empty());
    }
}

TEST_CASE("Range: Iterator operations")
{
    SECTION("range_iter returns correct values")
    {
        auto r = stx::range(0, 5, stx::range_dir::Forward);
        auto iter = r.begin();

        REQUIRE(*iter == 0);
        ++iter;
        REQUIRE(*iter == 1);
        ++iter;
        REQUIRE(*iter == 2);
    }

    SECTION("range_iter equality with sentinel")
    {
        auto r = stx::range(5, stx::range_dir::Forward);
        auto iter = r.begin();
        auto end = r.end();

        REQUIRE(iter != end);
        REQUIRE(*iter == 0);

        for (int i = 0; i < 5; ++i)
            ++iter;

        REQUIRE(iter == end);
    }
}
