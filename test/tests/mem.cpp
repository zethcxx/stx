#include <catch2/catch_test_macros.hpp>
#include <lbyte/stx/mem.hpp>

#include <array>
#include <cstring>
#include <span>

using namespace lbyte::stx;

TEST_CASE("Mem: Casting Utilities")
{
    SECTION("rcast performs reinterpret_cast")
    {
        int x = 42;
        void* ptr = &x;
        int* recovered = rcast<int*>(ptr);
        REQUIRE(recovered == &x);
    }

    SECTION("scast performs static_cast")
    {
        double d = 3.14;
        int i = scast<int>(d);
        REQUIRE(i == 3);
    }

    SECTION("bcast performs bit_cast")
    {
        u32 original = 0x12345678;
        u8 bytes[4];
        std::memcpy(bytes, &original, 4);

        u32 recovered = bcast<u32>(bytes);
        REQUIRE(recovered == original);
    }
}

TEST_CASE("Mem: read/write operations")
{
    alignas(16) std::array<std::byte, 64> buffer{};
    auto base = buffer.data();

    SECTION("read copies value from memory")
    {
        u32 expected = 0xDEADBEEF;
        std::memcpy(base, &expected, sizeof(expected));

        u32 value = read<u32>(base);
        REQUIRE(value == expected);
    }

    SECTION("read with offset")
    {
        buffer[8] = std::byte{0xFF};
        buffer[9] = std::byte{0xEE};

        u16 value = read<u16>(base, off_s{8});
        REQUIRE(value == 0xEEFF);
    }

    SECTION("read_raw dereferences memory directly")
    {
        u64 expected = 0x1122334455667788ULL;
        std::memcpy(base, &expected, sizeof(expected));

        u64 value = read_raw<u64>(base);
        REQUIRE(value == expected);
    }

    SECTION("write copies value to memory")
    {
        u32 value = 0xCAFEBABE;
        write<u32>(base, off_s{0}, value);

        u32 recovered;
        std::memcpy(&recovered, base, sizeof(recovered));
        REQUIRE(recovered == value);
    }

    SECTION("write with offset")
    {
        write<u16>(base, off_s{20}, 0x1234);

        REQUIRE(buffer[20] == std::byte{0x34});
        REQUIRE(buffer[21] == std::byte{0x12});
    }

    SECTION("write_raw assigns directly")
    {
        write_raw<u32>(base, off_s{0}, 0xAABBCCDD);

        u32 recovered;
        std::memcpy(&recovered, base, sizeof(recovered));
        REQUIRE(recovered == 0xAABBCCDD);
    }

    SECTION("read/write with va_s")
    {
        va_s va{reinterpret_cast<uptr>(base)};
        write<u32>(va, off_s{4}, 0x12345678);

        u32 value = read<u32>(va, off_s{4});
        REQUIRE(value == 0x12345678);
    }
}

TEST_CASE("Mem: Alignment with Strong Types")
{
    SECTION("align_up preserves strong type")
    {
        off_s off{123};
        auto aligned = align_up(off, 16u);
        static_assert(std::is_same_v<decltype(aligned), off_s>);
        REQUIRE(aligned.get() == 128);
    }

    SECTION("align_down preserves strong type")
    {
        off_s off{123};
        auto aligned = align_down(off, 16u);
        static_assert(std::is_same_v<decltype(aligned), off_s>);
        REQUIRE(aligned.get() == 112);
    }

    SECTION("rva_s alignment")
    {
        rva_s rva{100};
        auto aligned = align_up(rva, 4u);
        static_assert(std::is_same_v<decltype(aligned), rva_s>);
        REQUIRE(aligned.get() == 100);
    }

    SECTION("va_s alignment")
    {
        va_s va{0x1003};
        auto aligned = align_down(va, 0x1000u);
        static_assert(std::is_same_v<decltype(aligned), va_s>);
        REQUIRE(aligned.get() == 0x1000);
    }
}

TEST_CASE("Mem: Default offset parameter")
{
    alignas(16) std::array<std::byte, 32> buffer{};
    u32 expected = 0xDEAD;
    std::memcpy(buffer.data(), &expected, sizeof(expected));

    SECTION("read uses default offset of 0")
    {
        u32 value = read<u32>(buffer.data());
        REQUIRE(value == expected);
    }

    SECTION("read_raw uses default offset of 0")
    {
        u32 value = read_raw<u32>(buffer.data());
        REQUIRE(value == expected);
    }
}

TEST_CASE("Mem: Static Assertions")
{
    SECTION("read requires trivially copyable type")
    {
        struct NonTrivial {
            int x;
            ~NonTrivial() {}
        };
        (void)sizeof(NonTrivial);
    }
}
