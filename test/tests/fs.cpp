#include <catch2/catch_test_macros.hpp>
#include <lbyte/stx/fs.hpp>

#include <sstream>

using namespace lbyte;
#include <array>

TEST_CASE("Fs: dirty_vector")
{
    SECTION("dirty_vector default constructs elements")
    {
        stx::dirty_vector<stx::u32> vec(5);
        REQUIRE(vec.size() == 5);
        REQUIRE(vec.capacity() == 5);
    }

    SECTION("dirty_vector can hold binary_readable types")
    {
        stx::dirty_vector<stx::u64> vec(3);
        REQUIRE(vec.size() == 3);
    }

    SECTION("dirty_vector is a std::vector")
    {
        stx::dirty_vector<stx::u8> vec;
        static_assert(std::is_same_v<
            decltype(vec),
            std::vector<stx::u8, stx::details::vec_init_allocator<stx::u8>>
        >);
    }
}

TEST_CASE("Fs: setposfs")
{
    SECTION("setposfs seeks to beginning")
    {
        std::istringstream ss("Hello, World!");
        stx::setposfs(ss, stx::off_s{0}, stx::origin::begin);

        char c;
        ss.get(c);
        REQUIRE(c == 'H');
    }

    SECTION("setposfs seeks from current position")
    {
        std::istringstream ss("Hello, World!");

        ss.ignore(7);
        stx::setposfs(ss, stx::off_s{-5}, stx::origin::current);

        char c;
        ss.get(c);
        REQUIRE(c == 'o');
    }

    SECTION("setposfs seeks from end")
    {
        std::istringstream ss("Hello, World!");
        stx::setposfs(ss, stx::off_s{-6}, stx::origin::end);

        char c;
        ss.get(c);
        REQUIRE(c == 'W');
    }
}

TEST_CASE("Fs: readfs single value")
{
    SECTION("readfs reads single value from beginning")
    {
        std::array<stx::u8, 4> data{0xDE, 0xAD, 0xBE, 0xEF};
        std::istringstream ss(std::string(reinterpret_cast<char*>(data.data()), data.size()));

        stx::u32 value = stx::readfs<stx::u32>(ss);
        REQUIRE(value == 0xEFBEADDE);
    }

    SECTION("readfs with offset")
    {
        std::array<stx::u8, 8> data{0x00, 0x00, 0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x00};
        std::istringstream ss(std::string(reinterpret_cast<char*>(data.data()), data.size()));

        stx::u32 value = stx::readfs<stx::u32>(ss, stx::off_s{2});
        REQUIRE(value == 0xEFBEADDE);
    }

    SECTION("readfs with different origin")
    {
        std::array<stx::u8, 6> data{0x00, 0x00, 0xDE, 0xAD, 0xBE, 0xEF};
        std::istringstream ss(std::string(reinterpret_cast<char*>(data.data()), data.size()));

        ss.seekg(2);
        stx::u32 value = stx::readfs<stx::u32>(ss, stx::off_s{2}, stx::origin::current);
        REQUIRE(value == 0xEFBEADDE);
    }

    SECTION("readfs default parameters")
    {
        std::array<stx::u8, 4> data{0x12, 0x34, 0x56, 0x78};
        std::istringstream ss(std::string(reinterpret_cast<char*>(data.data()), data.size()));

        stx::u32 value = stx::readfs<stx::u32>(ss, stx::off_s{0}, stx::origin::begin);
        REQUIRE(value == 0x78563412);
    }
}

TEST_CASE("Fs: readfs into buffer")
{
    SECTION("readfs into span")
    {
        std::array<stx::u8, 8> data{0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE};
        std::istringstream ss(std::string(reinterpret_cast<char*>(data.data()), data.size()));

        std::array<stx::u32, 2> buffer{};
        stx::readfs<stx::u32>(ss, buffer, stx::off_s{0});

        REQUIRE(buffer[0] == 0xEFBEADDE);
        REQUIRE(buffer[1] == 0xBEBAFECA);
    }

    SECTION("readfs into span with offset")
    {
        std::array<stx::u8, 12> data{0x00, 0x00, 0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE, 0x00, 0x00};
        std::istringstream ss(std::string(reinterpret_cast<char*>(data.data()), data.size()));

        std::array<stx::u32, 2> buffer{};
        stx::readfs<stx::u32>(ss, buffer, stx::off_s{2});

        REQUIRE(buffer[0] == 0xEFBEADDE);
        REQUIRE(buffer[1] == 0xBEBAFECA);
    }
}

TEST_CASE("Fs: readfs into dirty_vector")
{
    SECTION("readfs returns dirty_vector")
    {
        std::array<stx::u8, 8> data{0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
        std::istringstream ss(std::string(reinterpret_cast<char*>(data.data()), data.size()));

        auto vec = stx::readfs<stx::u8>(ss, stx::off_s{0}, 8);
        REQUIRE(vec.size() == 8);
        REQUIRE(vec[0] == 0x01);
        REQUIRE(vec[7] == 0x08);
    }
}

TEST_CASE("Fs: readfs fixed-size array")
{
    SECTION("readfs into std::array")
    {
        std::array<stx::u8, 8> data{0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE};
        std::istringstream ss(std::string(reinterpret_cast<char*>(data.data()), data.size()));

        auto arr = stx::readfs<stx::u32, 2>(ss);
        REQUIRE(arr[0] == 0xEFBEADDE);
        REQUIRE(arr[1] == 0xBEBAFECA);
    }

    SECTION("readfs fixed array with offset")
    {
        std::array<stx::u8, 12> data{0x00, 0x00, 0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE, 0x00, 0x00};
        std::istringstream ss(std::string(reinterpret_cast<char*>(data.data()), data.size()));

        auto arr = stx::readfs<stx::u32, 2>(ss, stx::off_s{2});
        REQUIRE(arr[0] == 0xEFBEADDE);
        REQUIRE(arr[1] == 0xBEBAFECA);
    }
}

TEST_CASE("Fs: skipfs")
{
    SECTION("skipfs advances position")
    {
        std::istringstream ss("Hello, World!");

        stx::skipfs(ss, stx::off_s{7});

        char c;
        ss.get(c);
        REQUIRE(c == 'W');
    }

    SECTION("skipfs can skip backwards")
    {
        std::istringstream ss("Hello, World!");

        ss.seekg(10);
        stx::skipfs(ss, stx::off_s{-5});

        char c;
        ss.get(c);
        REQUIRE(c == 'o');
    }
}

TEST_CASE("Fs: last_read_ok")
{
    SECTION("last_read_ok returns true after successful read")
    {
        std::array<stx::u8, 4> data{0xDE, 0xAD, 0xBE, 0xEF};
        std::istringstream ss(std::string(reinterpret_cast<char*>(data.data()), data.size()));

        stx::readfs<stx::u32>(ss);
        REQUIRE(stx::last_read_ok(ss));
    }

    SECTION("last_read_ok returns false after failed read")
    {
        std::array<stx::u8, 2> data{0xDE, 0xAD};
        std::istringstream ss(std::string(reinterpret_cast<char*>(data.data()), data.size()));

        stx::readfs<stx::u32>(ss);
        REQUIRE_FALSE(stx::last_read_ok(ss));
    }
}

TEST_CASE("Fs: binary_readable types")
{
    SECTION("readfs works with struct")
    {
        struct [[gnu::packed]] header {
            stx::u32 magic;
            stx::u16 version;
            stx::u16 flags;
        };

        std::array<stx::u8, 8> data{0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x00, 0x00, 0x80};
        std::istringstream ss(std::string(reinterpret_cast<char*>(data.data()), data.size()));

        auto hdr = stx::readfs<header>(ss);
        REQUIRE(hdr.magic == 0xEFBEADDE);
        REQUIRE(hdr.version == 1);
        REQUIRE(hdr.flags == 0x8000);
    }
}
