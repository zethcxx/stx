#include <catch2/catch_test_macros.hpp>
#include <lbyte/stx/core.hpp>

#include <type_traits>

using namespace lbyte;

TEST_CASE("Core: Fundamental Type Aliases")
{
    SECTION("Unsigned integers have correct sizes")
    {
        static_assert(sizeof(stx::u8)  == 1);
        static_assert(sizeof(stx::u16) == 2);
        static_assert(sizeof(stx::u32) == 4);
        static_assert(sizeof(stx::u64) == 8);
    }

    SECTION("Signed integers have correct sizes")
    {
        static_assert(sizeof(stx::i8)  == 1);
        static_assert(sizeof(stx::i16) == 2);
        static_assert(sizeof(stx::i32) == 4);
        static_assert(sizeof(stx::i64) == 8);
    }

    SECTION("Floating point types have correct sizes")
    {
        static_assert(sizeof(stx::f32) == 4);
        static_assert(sizeof(stx::f64) == 8);
    }

    SECTION("Size types have platform-dependent sizes")
    {
        static_assert(sizeof(stx::usize) == sizeof(std::size_t));
        static_assert(sizeof(stx::isize) == sizeof(std::ptrdiff_t));
        static_assert(sizeof(stx::uptr)  == sizeof(std::uintptr_t));
        static_assert(sizeof(stx::iptr)  == sizeof(std::intptr_t));
    }
}

TEST_CASE("Core: Strong Types")
{
    SECTION("off_s construction and access")
    {
        stx::off_s off{128};
        REQUIRE(off.get() == 128);
        REQUIRE(static_cast<stx::usize>(off) == 128);
    }

    SECTION("rva_s construction and access")
    {
        stx::rva_s rva{0x2000};
        REQUIRE(rva.get() == 0x2000);
        REQUIRE(static_cast<stx::u32>(rva) == 0x2000);
    }

    SECTION("va_s construction and access")
    {
        stx::va_s va{0x140000000};
        REQUIRE(va.get() == 0x140000000);
        REQUIRE(static_cast<stx::uptr>(va) == 0x140000000);
    }

    SECTION("Strong type arithmetic - addition")
    {
        stx::off_s a{100};
        stx::off_s b{50};
        auto c = a + b;
        REQUIRE(c.get() == 150);

        auto d = a + 25;
        REQUIRE(d.get() == 125);
    }

    SECTION("Strong type arithmetic - subtraction")
    {
        stx::off_s a{100};
        stx::off_s b{30};
        auto diff = a - b;
        REQUIRE(diff == 70);

        auto e = a - 20;
        REQUIRE(e.get() == 80);
    }

    SECTION("Strong type compound assignment")
    {
        stx::off_s off{100};
        off += 50;
        REQUIRE(off.get() == 150);
        off -= 30;
        REQUIRE(off.get() == 120);
    }

    SECTION("Strong type comparison")
    {
        stx::off_s a{100};
        stx::off_s b{200};

        REQUIRE(a < b);
        REQUIRE(a <= b);
        REQUIRE(b > a);
        REQUIRE(b >= a);
        REQUIRE(a == stx::off_s{100});
        REQUIRE(a != b);

        auto cmp = (a <=> b);
        REQUIRE(cmp == std::strong_ordering::less);
    }

    SECTION("Cross-type subtraction")
    {
        stx::off_s a{100};
        stx::off_s b{30};
        stx::usize diff = a - b;
        REQUIRE(diff == 70);
    }
}

TEST_CASE("Core: Alignment Functions")
{
    SECTION("align_up rounds up to alignment boundary")
    {
        REQUIRE(stx::align_up(0u,  16u)  == 0u);
        REQUIRE(stx::align_up(1u,  16u)  == 16u);
        REQUIRE(stx::align_up(15u, 16u)  == 16u);
        REQUIRE(stx::align_up(16u, 16u)  == 16u);
        REQUIRE(stx::align_up(17u, 16u)  == 32u);
        REQUIRE(stx::align_up(123u, 16u) == 128u);
    }

    SECTION("align_down rounds down to alignment boundary")
    {
        REQUIRE(stx::align_down(0u,  16u)  == 0u);
        REQUIRE(stx::align_down(1u,  16u)  == 0u);
        REQUIRE(stx::align_down(15u, 16u)  == 0u);
        REQUIRE(stx::align_down(16u, 16u)  == 16u);
        REQUIRE(stx::align_down(17u, 16u)  == 16u);
        REQUIRE(stx::align_down(123u, 16u) == 112u);
    }

    SECTION("Strong type alignment preserves domain")
    {
        stx::off_s off{123};
        auto aligned = stx::align_up(off, 16u);
        static_assert(std::is_same_v<decltype(aligned), stx::off_s>);
        REQUIRE(aligned.get() == 128);

        auto down = stx::align_down(off, 16u);
        static_assert(std::is_same_v<decltype(down), stx::off_s>);
        REQUIRE(down.get() == 112);
    }
}

TEST_CASE("Core: Compile-Time Gap Calculators")
{
    SECTION("gap_v computes total size")
    {
        constexpr auto size1 = stx::gap_v<stx::u32, stx::u64, stx::u16>;
        static_assert(size1.get() == 14);

        constexpr auto size2 = stx::gap_v<stx::u8>;
        static_assert(size2.get() == 1);
    }

    SECTION("gap_align_v adds alignment padding")
    {
        constexpr auto aligned = stx::gap_align_v<8, stx::u32, stx::u16, stx::u64>;
        static_assert(aligned.get() == 16);
    }
}

TEST_CASE("Core: Concepts")
{
    SECTION("address_like concept")
    {
        int x = 0;
        static_assert(stx::address_like<decltype(&x)>);
        static_assert(stx::address_like<std::uintptr_t>);
        static_assert(stx::address_like<std::intptr_t>);
        static_assert(stx::address_like<stx::va_s>);
        static_assert(!stx::address_like<stx::off_s>);
        static_assert(!stx::address_like<stx::u32>);
    }

    SECTION("binary_readable concept")
    {
        struct valid {
            stx::u32 a;
            stx::u16 b;
        };
        static_assert(stx::binary_readable<valid>);
        static_assert(stx::binary_readable<stx::u32>);
        static_assert(stx::binary_readable<stx::u64>);

        struct with_virtual {
            virtual ~with_virtual() = default;
        };
        static_assert(!stx::binary_readable<with_virtual>);
        static_assert(!stx::binary_readable<int*>);
    }
}

TEST_CASE("Core: normalize_addr")
{
    int x = 42;
    stx::va_s va{0x140000000};

    SECTION("Normalizes pointers")
    {
        stx::uptr addr = stx::normalize_addr(&x);
        REQUIRE(addr == reinterpret_cast<stx::uptr>(&x));
    }

    SECTION("Normalizes va_s")
    {
        stx::uptr addr = stx::normalize_addr(va);
        REQUIRE(addr == 0x140000000);
    }

    SECTION("Normalizes uintptr_t")
    {
        stx::uptr addr = stx::normalize_addr(std::uintptr_t{0x1000});
        REQUIRE(addr == 0x1000);
    }
}

TEST_CASE("Core: Version")
{
    REQUIRE(stx::version.major == 2);
    REQUIRE(stx::version.minor == 0);
    REQUIRE(stx::version.patch == 0);
}

TEST_CASE("Core: origin enum")
{
    REQUIRE(stx::origin::begin    == stx::origin{0});
    REQUIRE(stx::origin::current  == stx::origin{1});
    REQUIRE(stx::origin::end      == stx::origin{2});
}
