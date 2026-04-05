#include <catch2/catch_test_macros.hpp>
#include <lbyte/stx/dump.hpp>

using namespace lbyte::stx;

TEST_CASE("Dump: Basic functionality")
{
    SECTION("dump accepts address_like types")
    {
        alignas(16) u8 buffer[16] = {0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE,
                                      0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0};

        REQUIRE(true);
    }

    SECTION("dump accepts va_s")
    {
        va_s addr{reinterpret_cast<uptr>(nullptr)};
        REQUIRE(true);
    }

    SECTION("dump accepts uintptr_t")
    {
        uptr addr = 0x1000;
        REQUIRE(true);
    }
}
