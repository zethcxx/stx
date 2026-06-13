#include <catch2/catch_test_macros.hpp>
#include <lbyte/stx/core.hpp>
#include <lbyte/stx/mem.hpp>

#include <span>
#include <type_traits>
#include <vector>

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
        REQUIRE(static_cast<stx::off_s::value_type>(off.get()) == 128);
        REQUIRE(static_cast<stx::off_s::value_type>(off) == 128);
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

    SECTION("strong_type cross-type conversion")
    {
        stx::rva_s rva{0x2000};
        stx::off_s from_rva{rva};
        REQUIRE(from_rva.get() == 0x2000);

        stx::rva_s from_off{stx::off_s{0x1000}};
        REQUIRE(from_off.get() == 0x1000);
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
        REQUIRE(stx::mem::align_up(0u,  16u)  == 0u);
        REQUIRE(stx::mem::align_up(1u,  16u)  == 16u);
        REQUIRE(stx::mem::align_up(15u, 16u)  == 16u);
        REQUIRE(stx::mem::align_up(16u, 16u)  == 16u);
        REQUIRE(stx::mem::align_up(17u, 16u)  == 32u);
        REQUIRE(stx::mem::align_up(123u, 16u) == 128u);
    }

    SECTION("align_down rounds down to alignment boundary")
    {
        REQUIRE(stx::mem::align_down(0u,  16u)  == 0u);
        REQUIRE(stx::mem::align_down(1u,  16u)  == 0u);
        REQUIRE(stx::mem::align_down(15u, 16u)  == 0u);
        REQUIRE(stx::mem::align_down(16u, 16u)  == 16u);
        REQUIRE(stx::mem::align_down(17u, 16u)  == 16u);
        REQUIRE(stx::mem::align_down(123u, 16u) == 112u);
    }

    SECTION("Strong type alignment preserves domain")
    {
        stx::off_s off{123};
        auto aligned = stx::mem::align_up(off, 16u);
        static_assert(std::is_same_v<decltype(aligned), stx::off_s>);
        REQUIRE(aligned.get() == 128);

        auto down = stx::mem::align_down(off, 16u);
        static_assert(std::is_same_v<decltype(down), stx::off_s>);
        REQUIRE(down.get() == 112);
    }
}

TEST_CASE("Core: Compile-Time Gap Calculators")
{
    SECTION("gap_v computes total size")
    {
        constexpr auto size1 = stx::mem::gap_v<stx::u32, stx::u64, stx::u16>;
        static_assert(size1.get() == 14);

        constexpr auto size2 = stx::mem::gap_v<stx::u8>;
        static_assert(size2.get() == 1);
    }

    SECTION("gap_align_v adds alignment padding")
    {
        constexpr auto aligned = stx::mem::gap_align_v<8, stx::u32, stx::u16, stx::u64>;
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
        static_assert(stx::address_like<stx::ptr<int>>);
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
    REQUIRE(stx::version.major == 1);
    REQUIRE(stx::version.minor == 0);
    REQUIRE(stx::version.patch == 0);
}

TEST_CASE("Core: defer")
{
    SECTION("LIFO order")
    {
        int counter = 0;
        {
            auto _d1 = stx::defer{ [&] { counter = 3; } };
            auto _d2 = stx::defer{ [&] { counter = 2; } };
            auto _d3 = stx::defer{ [&] { counter = 1; } };
        }
        REQUIRE(counter == 3);
    }

    SECTION("cancel")
    {
        int executed = 0;
        {
            auto d = stx::defer{ [&] { ++executed; } };
            d.cancel();
        }
        REQUIRE(executed == 0);
    }

    SECTION("no cancel = executes")
    {
        int executed = 0;
        {
            auto d = stx::defer{ [&] { ++executed; } };
            (void)d;
        }
        REQUIRE(executed == 1);
    }

    SECTION("selective cancel with named defers")
    {
        int a = 0, b = 0;
        {
            auto _d1 = stx::defer{ [&] { a = 1; } };
            auto _d2 = stx::defer{ [&] { b = 1; } };
            _d1.cancel();
        }
        REQUIRE(a == 0);
        REQUIRE(b == 1);
    }
}

TEST_CASE("Core: null_t")
{
    SECTION("converts to uptr 0")
    {
        stx::uptr u = stx::null;
        REQUIRE(u == 0);
    }

    SECTION("converts to nullptr_t")
    {
        std::nullptr_t np = static_cast<std::nullptr_t>(stx::null);
        REQUIRE(np == nullptr);
    }

    SECTION("contextually false")
    {
        REQUIRE_FALSE(stx::null);
    }

    SECTION("suppress nodiscard via <<")
    {
        [[maybe_unused]] auto marker = []() -> int { return 42; };
        stx::null << marker();
        // compilation test: must not produce nodiscard warning
        REQUIRE(true);
    }

    SECTION("chaining suppresses multiple values")
    {
        stx::null << 1 << 2u << 3ull;
        REQUIRE(true);
    }
}

TEST_CASE("Core: ccast")
{
    SECTION("Removes const from const int")
    {
        const int x = 42;
        int* p = stx::ccast<int*>(&x);
        REQUIRE(*p == 42);
    }
}

#ifdef __GXX_RTTI
TEST_CASE("Core: dcast")
{
    SECTION("Polymorphic downcast")
    {
        struct Base { virtual ~Base() = default; };
        struct Derived : Base { int value = 42; };
        Base* b = new Derived();
        auto* d = stx::dcast<Derived*>(b);
        REQUIRE(d->value == 42);
        delete d;
    }
}
#endif

TEST_CASE("Core: strong_type cross-tag compound assignment")
{
    SECTION("va_s += off_s")
    {
        stx::va_s va{0x140000000};
        stx::off_s off{128};
        va += off;
        REQUIRE(va.get() == 0x140000080);
    }

    SECTION("rva_s += off_s")
    {
        stx::rva_s rva{0x2000};
        stx::off_s off{256};
        rva += off;
        REQUIRE(rva.get() == 0x2100);
    }

    SECTION("va_s -= off_s")
    {
        stx::va_s va{0x140000100};
        stx::off_s off{256};
        va -= off;
        REQUIRE(va.get() == 0x140000000);
    }

    SECTION("rva_s -= off_s")
    {
        stx::rva_s rva{0x2100};
        stx::off_s off{256};
        rva -= off;
        REQUIRE(rva.get() == 0x2000);
    }
}

TEST_CASE("Core: strong_type increment/decrement")
{
    SECTION("rva_s prefix ++")
    {
        stx::rva_s rva{0x2000};
        auto& ref = ++rva;
        REQUIRE(rva.get() == 0x2001);
        REQUIRE(&ref == &rva);
    }

    SECTION("rva_s postfix ++")
    {
        stx::rva_s rva{0x2000};
        auto old = rva++;
        REQUIRE(old.get() == 0x2000);
        REQUIRE(rva.get() == 0x2001);
    }

    SECTION("rva_s prefix --")
    {
        stx::rva_s rva{0x2000};
        --rva;
        REQUIRE(rva.get() == 0x1FFF);
    }

    SECTION("rva_s postfix --")
    {
        stx::rva_s rva{0x2000};
        auto old = rva--;
        REQUIRE(old.get() == 0x2000);
        REQUIRE(rva.get() == 0x1FFF);
    }

    SECTION("va_s prefix ++")
    {
        stx::va_s va{0x140000000};
        ++va;
        REQUIRE(va.get() == 0x140000001);
    }

    SECTION("va_s postfix ++")
    {
        stx::va_s va{0x140000000};
        auto old = va++;
        REQUIRE(old.get() == 0x140000000);
        REQUIRE(va.get() == 0x140000001);
    }

    SECTION("va_s prefix --")
    {
        stx::va_s va{0x140000001};
        --va;
        REQUIRE(va.get() == 0x140000000);
    }

    SECTION("va_s postfix --")
    {
        stx::va_s va{0x140000001};
        auto old = va--;
        REQUIRE(old.get() == 0x140000001);
        REQUIRE(va.get() == 0x140000000);
    }
}

TEST_CASE("Core: byte_swappable with enums")
{
    SECTION("Simple enum satisfies the concept")
    {
        enum class color : stx::u32 { red, green, blue };
        static_assert(stx::byte_swappable<color>);
        static_assert(!stx::byte_swappable<color&>);
        static_assert(!stx::byte_swappable<bool>);
        static_assert(!stx::byte_swappable<char>);
    }
}

TEST_CASE("Core: writable_buffer concept")
{
    SECTION("std::span<std::byte>")
    {
        std::array<std::byte, 4> arr{};
        std::span<std::byte> sp{arr};
        static_assert(stx::writable_buffer<decltype(sp)>);
    }

    SECTION("std::vector<std::byte>")
    {
        static_assert(stx::writable_buffer<std::vector<std::byte>>);
    }

    SECTION("std::array<std::byte, N>")
    {
        static_assert(stx::writable_buffer<std::array<std::byte, 4>>);
    }
}

TEST_CASE("Core: Cross-tag operators with rva_s")
{
    SECTION("rva_s + off_s")
    {
        stx::rva_s rva{0x2000};
        stx::off_s off{128};
        auto result = rva + off;
        REQUIRE(result.get() == 0x2080);
    }

    SECTION("off_s + rva_s")
    {
        stx::off_s off{128};
        stx::rva_s rva{0x2000};
        auto result = off + rva;
        REQUIRE(result.get() == 128 + 0x2000);
    }

    SECTION("rva_s - off_s")
    {
        stx::rva_s rva{0x2100};
        stx::off_s off{256};
        auto result = rva - off;
        REQUIRE(result.get() == 0x2000);
    }

    SECTION("off_s + va_s")
    {
        stx::off_s off{100};
        stx::va_s va{0x1000};
        auto result = off + va;
        REQUIRE(result.get() == 100 + 0x1000);
    }
}

TEST_CASE("Core: bounded_array concept")
{
    SECTION("Various array types")
    {
        static_assert(stx::bounded_array<int[4]>);
        static_assert(stx::bounded_array<stx::u32[8]>);
        static_assert(stx::bounded_array<int[2][3]>);
        static_assert(!stx::bounded_array<int[]>);
        static_assert(!stx::bounded_array<int*>);
        static_assert(!stx::bounded_array<int>);
    }
}

