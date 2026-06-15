#include <catch2/catch_test_macros.hpp>
#include <lbyte/stx/fs.hpp>

#include <fstream>
#include <sstream>

using namespace lbyte;
using namespace lbyte::stx::fs;
#include <array>

TEST_CASE("Fs: dirty_vector")
{
    SECTION("dirty_vector default constructs elements")
    {
        stx::fs::dirty_vector<stx::u32> vec(5);
        REQUIRE(vec.size() == 5);
        REQUIRE(vec.capacity() == 5);
    }

    SECTION("dirty_vector can hold binary_readable types")
    {
        stx::fs::dirty_vector<stx::u64> vec(3);
        REQUIRE(vec.size() == 3);
    }

    SECTION("dirty_vector is a std::vector")
    {
        stx::fs::dirty_vector<stx::u8> vec;
        static_assert(std::is_same_v<
            decltype(vec),
            std::vector<stx::u8, stx::details::vec_init_allocator<stx::u8>>
        >);
    }
}

TEST_CASE("Fs: origin enum")
{
    REQUIRE(stx::fs::origin::begin    == stx::fs::origin{0});
    REQUIRE(stx::fs::origin::current  == stx::fs::origin{1});
    REQUIRE(stx::fs::origin::end      == stx::fs::origin{2});
}

TEST_CASE("Fs: setpos (istream)")
{
    SECTION("setpos seeks to beginning")
    {
        std::istringstream ss("Hello, World!");
        setpos(ss, stx::off_s{0}, stx::fs::origin::begin);

        char c;
        ss.get(c);
        REQUIRE(c == 'H');
    }

    SECTION("setpos seeks from current position")
    {
        std::istringstream ss("Hello, World!");

        ss.ignore(7);
        setpos(ss, stx::off_s{-3}, stx::fs::origin::current);

        char c;
        ss.get(c);
        REQUIRE(c == 'o');
    }

    SECTION("setpos seeks from end")
    {
        std::istringstream ss("Hello, World!");
        setpos(ss, stx::off_s{-6}, stx::fs::origin::end);

        char c;
        ss.get(c);
        REQUIRE(c == 'W');
    }
}

TEST_CASE("Fs: read single value")
{
    SECTION("read reads single value from beginning")
    {
        std::array<stx::u8, 4> data{0xDE, 0xAD, 0xBE, 0xEF};
        std::istringstream ss(std::string(reinterpret_cast<char*>(data.data()), data.size()));

        auto result = read<stx::u32>(ss);
        REQUIRE(result);
        REQUIRE(*result == 0xEFBEADDE);
    }

    SECTION("read with offset")
    {
        std::array<stx::u8, 8> data{0x00, 0x00, 0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x00};
        std::istringstream ss(std::string(reinterpret_cast<char*>(data.data()), data.size()));

        auto result = read<stx::u32>(ss, stx::off_s{2});
        REQUIRE(result);
        REQUIRE(*result == 0xEFBEADDE);
    }

    SECTION("read with different origin")
    {
        std::array<stx::u8, 8> data{0x00, 0x00, 0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x00};
        std::istringstream ss(std::string(reinterpret_cast<char*>(data.data()), data.size()));

        ss.seekg(2);
        auto result = read<stx::u32>(ss, stx::off_s{0}, stx::fs::origin::current);
        REQUIRE(result);
        REQUIRE(*result == 0xEFBEADDE);
    }

    SECTION("read default parameters")
    {
        std::array<stx::u8, 4> data{0x12, 0x34, 0x56, 0x78};
        std::istringstream ss(std::string(reinterpret_cast<char*>(data.data()), data.size()));

        auto result = read<stx::u32>(ss, stx::off_s{0}, stx::fs::origin::begin);
        REQUIRE(result);
        REQUIRE(*result == 0x78563412);
    }
}

TEST_CASE("Fs: read into buffer")
{
    SECTION("read into span")
    {
        std::array<stx::u8, 8> data{0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE};
        std::istringstream ss(std::string(reinterpret_cast<char*>(data.data()), data.size()));

        std::array<stx::u32, 2> buffer{};
        auto result = read<stx::u32>(ss, buffer, stx::off_s{0});
        REQUIRE(result);

        REQUIRE(buffer[0] == 0xEFBEADDE);
        REQUIRE(buffer[1] == 0xBEBAFECA);
    }

    SECTION("read into span with offset")
    {
        std::array<stx::u8, 12> data{0x00, 0x00, 0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE, 0x00, 0x00};
        std::istringstream ss(std::string(reinterpret_cast<char*>(data.data()), data.size()));

        std::array<stx::u32, 2> buffer{};
        auto result = read<stx::u32>(ss, buffer, stx::off_s{2});
        REQUIRE(result);

        REQUIRE(buffer[0] == 0xEFBEADDE);
        REQUIRE(buffer[1] == 0xBEBAFECA);
    }
}

TEST_CASE("Fs: read into dirty_vector")
{
    SECTION("read returns dirty_vector")
    {
        std::array<stx::u8, 8> data{0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
        std::istringstream ss(std::string(reinterpret_cast<char*>(data.data()), data.size()));

        auto result = read<stx::u8>(ss, stx::off_s{0}, 8);
        REQUIRE(result);
        auto& vec = *result;
        REQUIRE(vec.size() == 8);
        REQUIRE(vec[0] == 0x01);
        REQUIRE(vec[7] == 0x08);
    }
}

TEST_CASE("Fs: read fixed-size array")
{
    SECTION("read into std::array")
    {
        std::array<stx::u8, 8> data{0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE};
        std::istringstream ss(std::string(reinterpret_cast<char*>(data.data()), data.size()));

        auto result = read<stx::u32, 2>(ss);
        REQUIRE(result);
        auto& arr = *result;
        REQUIRE(arr[0] == 0xEFBEADDE);
        REQUIRE(arr[1] == 0xBEBAFECA);
    }

    SECTION("read fixed array with offset")
    {
        std::array<stx::u8, 12> data{0x00, 0x00, 0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE, 0x00, 0x00};
        std::istringstream ss(std::string(reinterpret_cast<char*>(data.data()), data.size()));

        auto result = read<stx::u32, 2>(ss, stx::off_s{2});
        REQUIRE(result);
        auto& arr = *result;
        REQUIRE(arr[0] == 0xEFBEADDE);
        REQUIRE(arr[1] == 0xBEBAFECA);
    }
}

TEST_CASE("Fs: advance istream")
{
    SECTION("advance advances position")
    {
        std::istringstream ss("Hello, World!");

        stx::fs::advance(ss, stx::off_s{7});

        char c;
        ss.get(c);
        REQUIRE(c == 'W');
    }

    SECTION("advance can skip backwards")
    {
        std::istringstream ss("Hello, World!");

        ss.seekg(9);
        stx::fs::advance(ss, stx::off_s{-5});

        char c;
        ss.get(c);
        REQUIRE(c == 'o');
    }
}

TEST_CASE("Fs: binary_readable types")
{
    SECTION("read works with struct")
    {
        struct [[gnu::packed]] header {
            stx::u32 magic;
            stx::u16 version;
            stx::u16 flags;
        };

        std::array<stx::u8, 8> data{0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x00, 0x00, 0x80};
        std::istringstream ss(std::string(reinterpret_cast<char*>(data.data()), data.size()));

        auto r = read<header>(ss);
        REQUIRE(r);
        REQUIRE(r->magic == 0xEFBEADDE);
        REQUIRE(r->version == 1);
        REQUIRE(r->flags == 0x8000);
    }
}

TEST_CASE("Fs: setpos (ostream)")
{
    SECTION("setpos seeks from beginning")
    {
        std::ostringstream ss;
        ss << "Hello, World!";

        setpos(ss, stx::off_s{7});
        ss << "STX";
        REQUIRE(ss.view() == "Hello, STXld!");
    }

    SECTION("setpos seeks from end")
    {
        std::ostringstream ss;
        ss << "Hello, World!";

        setpos(ss, stx::off_s{0}, stx::fs::origin::end);
        ss << "!!";
        REQUIRE(ss.view() == "Hello, World!!!");
    }
}

TEST_CASE("Fs: write")
{
    SECTION("write writes single value")
    {
        std::ostringstream ss;

        auto r = write(ss, stx::off_s{0}, stx::u32{0xEFBEADDE});
        REQUIRE(r);

        std::array<unsigned char, 4> expected{0xDE, 0xAD, 0xBE, 0xEF};
        auto view = ss.view();
        REQUIRE(view.size() >= 4);
        REQUIRE(std::memcmp(view.data(), expected.data(), 4) == 0);
    }

    SECTION("write writes span")
    {
        std::ostringstream ss;
        std::array<stx::u32, 2> data{0xEFBEADDE, 0xBEBAFECA};
        std::span<const stx::u32> sp{data};

        auto r = write(ss, stx::off_s{0}, sp);
        REQUIRE(r);

        auto view = ss.view();
        REQUIRE(view.size() >= 8);
        stx::u32 v0, v1;
        std::memcpy(&v0, view.data(), 4);
        std::memcpy(&v1, view.data() + 4, 4);
        REQUIRE(v0 == 0xEFBEADDE);
        REQUIRE(v1 == 0xBEBAFECA);
    }

    SECTION("write writes at offset")
    {
        std::ostringstream ss;
        ss << "XXXXXXXX";

        auto r = write(ss, stx::off_s{2}, stx::u32{0xEFBEADDE});
        REQUIRE(r);

        auto view = ss.view();
        REQUIRE(view.size() >= 6);
        stx::u32 val;
        std::memcpy(&val, view.data() + 2, 4);
        REQUIRE(val == 0xEFBEADDE);
    }
}

TEST_CASE("Fs: advance ostream")
{
    SECTION("advance advances write position")
    {
        std::ostringstream ss;
        ss << "Hello, World!";

        stx::fs::advance(ss, stx::off_s{5});
        ss << "XXX";

        auto view = ss.view();
        REQUIRE(view.size() >= 13);
        REQUIRE(view[0] == 'H');
        REQUIRE(view[1] == 'e');
    }

    SECTION("advance can skip backwards")
    {
        std::ostringstream ss;
        ss << "Hello, World!";

        setpos(ss, stx::off_s{7});
        stx::fs::advance(ss, stx::off_s{-5});
        ss << "XXX";

        auto view = ss.view();
        REQUIRE(view.substr(0, 10) == "HeXXX, Wor");
    }
}

TEST_CASE("Fs: map_file")
{
    SECTION("open and read from file")
    {
        auto path = std::filesystem::temp_directory_path() / "stx_test_map.bin";
        std::error_code ec;

        // write test data
        {
            std::ofstream ofs(path, std::ios::binary);
            REQUIRE(ofs);
            stx::u32 data = 0xDEADBEEF;
            ofs.write(reinterpret_cast<const char*>(&data), sizeof(data));
        }

        auto mapped = stx::map_file::open(path);
        REQUIRE(mapped);
        auto& m = *mapped;
        REQUIRE(m);
        REQUIRE(m.size() == 4);

        stx::u32 val = (m.as_p<stx::u32>() + stx::off_s{0}).read<stx::u32>();
        REQUIRE(val == 0xDEADBEEF);

        std::filesystem::remove(path, ec);
    }

    SECTION("open with offset and size")
    {
        auto path = std::filesystem::temp_directory_path() / "stx_test_map2.bin";
        std::error_code ec;

        {
            std::ofstream ofs(path, std::ios::binary);
            REQUIRE(ofs);
            std::array<stx::u8, 16> data{};
            data[8] = 0x42;
            ofs.write(reinterpret_cast<const char*>(data.data()), data.size());
        }

        auto mapped = stx::map_file::open(path, stx::off_s{4}, 8);
        REQUIRE(mapped);
        auto& m = *mapped;
        REQUIRE(m.size() == 8);
        REQUIRE(m.base() != 0);

        stx::u8 val = (m.as_p<stx::u8>() + stx::off_s{4}).read<stx::u8>();
        REQUIRE(val == 0x42);

        std::filesystem::remove(path, ec);
    }

    SECTION("open fails on nonexistent file")
    {
        auto result = stx::map_file::open("/nonexistent/stx_test_file.bin");
        REQUIRE_FALSE(result);
    }

    SECTION("open with write flag and modify")
    {
        auto path = std::filesystem::temp_directory_path() / "stx_test_map3.bin";
        std::error_code ec;

        {
            std::ofstream ofs(path, std::ios::binary);
            REQUIRE(ofs);
            stx::u32 data = 0x12345678;
            ofs.write(reinterpret_cast<const char*>(&data), sizeof(data));
        }

        auto flags = stx::map_flag::write;
        auto mapped = stx::map_file::open(path, flags);
        REQUIRE(mapped);
        auto& m = *mapped;

        (m.as_p<stx::u32>() + stx::off_s{0}).write(stx::u32{0xDEADBEEF});

        // verify via read
        stx::u32 val = (m.as_p<stx::u32>() + stx::off_s{0}).read<stx::u32>();
        REQUIRE(val == 0xDEADBEEF);

        std::filesystem::remove(path, ec);
    }

    SECTION("sequential I/O")
    {
        auto path = std::filesystem::temp_directory_path() / "stx_test_map_seq.bin";
        std::error_code ec;

        {
            std::ofstream ofs(path, std::ios::binary);
            REQUIRE(ofs);
            std::array<stx::u32, 4> data{10, 20, 30, 40};
            ofs.write(reinterpret_cast<const char*>(data.data()), data.size() * sizeof(stx::u32));
        }

        auto mapped = stx::map_file::open(path);
        REQUIRE(mapped);
        auto& m = *mapped;

        REQUIRE(m.tell() == stx::off_s{0});
        REQUIRE(m.remaining() == stx::off_s{16});

        auto v0 = m.pop<stx::u32>();
        REQUIRE(v0 == 10);
        REQUIRE(m.tell() == stx::off_s{4});

        auto v1 = m.pop<stx::u32>();
        REQUIRE(v1 == 20);
        REQUIRE(m.tell() == stx::off_s{8});

        m.seek(stx::off_s{0});
        REQUIRE(m.tell() == stx::off_s{0});

        std::filesystem::remove(path, ec);
    }

    SECTION("span and as_p")
    {
        auto path = std::filesystem::temp_directory_path() / "stx_test_map_span.bin";
        std::error_code ec;

        {
            std::ofstream ofs(path, std::ios::binary);
            REQUIRE(ofs);
            stx::u32 data = 0xCAFEBABE;
            ofs.write(reinterpret_cast<const char*>(&data), sizeof(data));
        }

        auto mapped = stx::map_file::open(path);
        REQUIRE(mapped);
        auto& m = *mapped;

        auto b = m.bytes();
        REQUIRE(b.size() == 4);

        auto p = m.as_p<stx::u32>();
        REQUIRE(p.addr() == m.base());
        REQUIRE(p.read<stx::u32>() == 0xCAFEBABE);

        std::filesystem::remove(path, ec);
    }

    SECTION("read overload for map_file")
    {
        auto path = std::filesystem::temp_directory_path() / "stx_test_map_rfs.bin";
        std::error_code ec;

        {
            std::ofstream ofs(path, std::ios::binary);
            REQUIRE(ofs);
            stx::u32 data = 0xAABBCCDD;
            ofs.write(reinterpret_cast<const char*>(&data), sizeof(data));
        }

        auto mapped = stx::map_file::open(path);
        REQUIRE(mapped);

        auto r = read<stx::u32>(*mapped, stx::off_s{0});
        REQUIRE(r);
        REQUIRE(*r == 0xAABBCCDD);

        std::filesystem::remove(path, ec);
    }

    SECTION("move semantics")
    {
        auto path = std::filesystem::temp_directory_path() / "stx_test_map_move.bin";
        std::error_code ec;

        {
            std::ofstream ofs(path, std::ios::binary);
            REQUIRE(ofs);
            stx::u32 data = 0x12345678;
            ofs.write(reinterpret_cast<const char*>(&data), sizeof(data));
        }

        auto m1 = stx::map_file::open(path);
        REQUIRE(m1);

        auto m2 = std::move(*m1);
        REQUIRE(m2);
        REQUIRE((m2.as_p<stx::u32>() + stx::off_s{0}).read<stx::u32>() == 0x12345678);

        std::filesystem::remove(path, ec);
    }
}

TEST_CASE("Fs: memcur")
{
    SECTION("default constructed is empty")
    {
        stx::memcur r;
        REQUIRE_FALSE(r);
        REQUIRE(r.size() == 0);
    }

    SECTION("buffer construction")
    {
        std::array<std::byte, 4> buf{};
        stx::memcur r{std::span<std::byte>(buf)};
        REQUIRE(r);
        REQUIRE(r.size() == 4);
    }

    SECTION("read single value")
    {
        stx::u32 val = 0xDEADBEEF;
        stx::memcur r(&val, sizeof(val));

        auto v = r.pop<stx::u32>();
        REQUIRE(v == 0xDEADBEEF);
        REQUIRE(r.tell() == stx::off_s{4});
        REQUIRE(r.remaining() == stx::off_s{0});
    }

    SECTION("seek and advance")
    {
        stx::u32 data[] = {1, 2, 3, 4};
        stx::memcur r(data, sizeof(data));

        REQUIRE(r.pop<stx::u32>() == 1);
        r.seek(stx::off_s{0});
        REQUIRE(r.pop<stx::u32>() == 1);

        r.seek(stx::off_s{4});
        REQUIRE(r.pop<stx::u32>() == 2);

        r.seek(stx::off_s{-4}, stx::fs::origin::end);
        REQUIRE(r.pop<stx::u32>() == 4);
    }

    SECTION("read into fixed array")
    {
        stx::u32 data[] = {100, 200};
        stx::memcur r(data, sizeof(data));

        auto arr = r.pop<stx::u32[2]>();
        REQUIRE(arr[0] == 100);
        REQUIRE(arr[1] == 200);
    }

    SECTION("as_view zero-copy (no advance)")
    {
        stx::u32 data[] = {42, 43, 44};
        stx::memcur r(data, sizeof(data));

        auto view = r.as_view<stx::u32>(3);
        REQUIRE(view.size() == 3);
        REQUIRE(view[0] == 42);
        REQUIRE(view[2] == 44);
        REQUIRE(r.tell() == stx::off_s{0});
    }

    SECTION("read_strvw bounded")
    {
        std::byte buf[] = {
            std::byte{'h'}, std::byte{'i'}, std::byte{'\0'},
            std::byte{'x'}
        };
        stx::memcur r{std::span<std::byte>(buf)};

        auto s = r.read_strvw(10);
        REQUIRE(s == "hi");
        REQUIRE(r.tell() == stx::off_s{3});
    }

    SECTION("read_strvw unbounded")
    {
        std::byte buf[] = {
            std::byte{'a'}, std::byte{'b'}, std::byte{'c'}, std::byte{'\0'}
        };
        stx::memcur r{std::span<std::byte>(buf)};

        auto s = r.read_strvw();
        REQUIRE(s == "abc");
        REQUIRE(r.tell() == stx::off_s{4});
    }

    SECTION("read_strvw reaches end without null")
    {
        std::byte buf[] = {std::byte{'x'}, std::byte{'y'}};
        stx::memcur r{std::span<std::byte>(buf)};

        auto s = r.read_strvw();
        REQUIRE(s == "xy");
        REQUIRE(r.tell() == stx::off_s{2});
    }

    SECTION("write single value")
    {
        std::array<std::byte, 4> buf{};
        stx::memcur r{std::span<std::byte>(buf)};

        r.push(stx::u32{0x12345678});
        REQUIRE(r.tell() == stx::off_s{4});

        r.seek(stx::off_s{0});
        REQUIRE(r.pop<stx::u32>() == 0x12345678);
    }

    SECTION("write span")
    {
        std::array<std::byte, 8> buf{};
        stx::memcur r{std::span<std::byte>(buf)};

        stx::u32 vals[] = {1, 2};
        r.push(vals);
        REQUIRE(r.tell() == stx::off_s{8});

        r.seek(stx::off_s{0});
        REQUIRE(r.pop<stx::u32>() == 1);
        REQUIRE(r.pop<stx::u32>() == 2);
    }

    SECTION("push string_view")
    {
        std::array<std::byte, 32> buf{};
        stx::memcur r{std::span<std::byte>(buf)};

        r.push(std::string_view{"hello world"});
        REQUIRE(r.tell() == stx::off_s{11});

        r.seek(stx::off_s{0});
        stx::u8 data[11];
        std::memcpy(data, r.as_p<stx::u8>().raw(), 11);
        auto const* p = reinterpret_cast<const char*>(data);

        REQUIRE(std::string_view(p, p + 11) == "hello world");
    }

    SECTION("positional read")
    {
        stx::u32 val = 0xAABBCCDD;
        stx::memcur r(&val, sizeof(val));

        auto v = (r.as_p<stx::u32>() + stx::off_s{0}).read<stx::u32>();
        REQUIRE(v == 0xAABBCCDD);
        REQUIRE(r.tell() == stx::off_s{0});
    }

    SECTION("positional write")
    {
        std::array<std::byte, 4> buf{};
        stx::memcur r{std::span<std::byte>(buf)};
        (r.as_p<stx::u32>() + stx::off_s{0}).write(stx::u32{0x55555555});
        REQUIRE(r.tell() == stx::off_s{0});

        REQUIRE((r.as_p<stx::u32>() + stx::off_s{0}).read<stx::u32>() == 0x55555555);
    }

    SECTION("bytes() and as_p()")
    {
        stx::u32 val = 0xCAFEBABE;
        stx::memcur r(&val, sizeof(val));

        auto b = r.bytes();
        REQUIRE(b.size() == 4);

        auto p = r.as_p<stx::u32>();
        REQUIRE(p.read<stx::u32>() == 0xCAFEBABE);
    }
}

TEST_CASE("Fs: map_file new methods")
{
    SECTION("read<T>(count) into vector")
    {
        auto path = std::filesystem::temp_directory_path() / "stx_test_map_vec.bin";
        std::error_code ec;

        {
            std::ofstream ofs(path, std::ios::binary);
            REQUIRE(ofs);
            stx::u32 data[] = {1, 2, 3, 4};
            ofs.write(reinterpret_cast<const char*>(data), sizeof(data));
        }

        auto mapped = stx::map_file::open(path);
        REQUIRE(mapped);
        auto& m = *mapped;

        SECTION("by array size")
        {
            auto arr = m.pop<stx::u32[4]>();
            REQUIRE(arr[0] == 1);
            REQUIRE(arr[3] == 4);
        }

        std::filesystem::remove(path, ec);
    }

    SECTION("write span")
    {
        auto path = std::filesystem::temp_directory_path() / "stx_test_map_wspan.bin";
        std::error_code ec;

        {
            std::ofstream ofs(path, std::ios::binary);
            REQUIRE(ofs);
            std::byte zero[8]{};
            ofs.write(reinterpret_cast<const char*>(zero), 8);
        }

        auto mapped = stx::map_file::open(path, stx::map_flag::write);
        REQUIRE(mapped);
        auto& m = *mapped;

        stx::u32 vals[] = {0xDEAD, 0xBEEF};
        m.push(vals);
        m.seek(stx::off_s{0});
        REQUIRE(m.pop<stx::u32>() == 0xDEAD);
        REQUIRE(m.pop<stx::u32>() == 0xBEEF);

        std::filesystem::remove(path, ec);
    }

    SECTION("push string_view on map_file")
    {
        auto path = std::filesystem::temp_directory_path() / "stx_test_map_push_sv.bin";
        std::error_code ec;

        {
            std::ofstream ofs(path, std::ios::binary);
            REQUIRE(ofs);
            std::byte zero[32]{};
            ofs.write(reinterpret_cast<const char*>(zero), 32);
        }

        auto mapped = stx::map_file::open(path, stx::map_flag::write);
        REQUIRE(mapped);
        auto& m = *mapped;

        m.push(std::string_view{"map push sv"});
        REQUIRE(m.tell() == stx::off_s{11});

        m.seek(stx::off_s{0});
        stx::u8 data[11];
        std::memcpy(data, m.as_p<void>().raw(), 11);
        auto const* p = reinterpret_cast<const char*>(data);
        REQUIRE(std::string_view(p, p + 11) == "map push sv");

        std::filesystem::remove(path, ec);
    }

    SECTION("read_strvw and as_view in map_file")
    {
        auto path = std::filesystem::temp_directory_path() / "stx_test_map_strvw.bin";
        std::error_code ec;

        {
            std::ofstream ofs(path, std::ios::binary);
            REQUIRE(ofs);
            std::byte data[] = {
                std::byte{'h'}, std::byte{'e'}, std::byte{'l'}, std::byte{'l'},
                std::byte{'o'}, std::byte{'\0'},
                std::byte{0x01}, std::byte{0x02}
            };
            ofs.write(reinterpret_cast<const char*>(data), sizeof(data));
        }

        auto mapped = stx::map_file::open(path);
        REQUIRE(mapped);
        auto& m = *mapped;

        auto s = m.read_strvw();
        REQUIRE(s == "hello");
        REQUIRE(m.tell() == stx::off_s{6});

        auto view = m.as_view<std::byte>(2);
        REQUIRE(view.size() == 2);
        REQUIRE(view[0] == std::byte{0x01});
        REQUIRE(m.tell() == stx::off_s{6});

        std::filesystem::remove(path, ec);
    }

    SECTION("seek with origin::end")
    {
        auto path = std::filesystem::temp_directory_path() / "stx_test_map_end.bin";
        std::error_code ec;

        {
            std::ofstream ofs(path, std::ios::binary);
            REQUIRE(ofs);
            stx::u32 data = 0xFFFFFFFF;
            ofs.write(reinterpret_cast<const char*>(&data), sizeof(data));
        }

        auto mapped = stx::map_file::open(path);
        REQUIRE(mapped);
        auto& m = *mapped;

        m.seek(stx::off_s{-2}, stx::fs::origin::end);
        REQUIRE(m.tell() == stx::off_s{2});

        std::filesystem::remove(path, ec);
    }
}

TEST_CASE("Fs: memcur new API")
{
    SECTION("pop<T[N]> array")
    {
        stx::u32 data[] = {1, 2, 3};
        stx::memcur r(data, sizeof(data));

        auto arr = r.pop<stx::u32[3]>();
        REQUIRE(arr[0] == 1);
        REQUIRE(arr[2] == 3);
        REQUIRE(r.tell() == stx::off_s{12});
    }

    SECTION("as_view zero-copy (no advance)")
    {
        stx::u32 data[] = {10, 20, 30};
        stx::memcur r(data, sizeof(data));

        auto view = r.as_view<stx::u32[3]>();
        REQUIRE(view.size() == 3);
        REQUIRE(view[1] == 20);
        REQUIRE(r.tell() == stx::off_s{0});
    }

    SECTION("push generic span")
    {
        std::array<std::byte, 12> buf{};
        stx::memcur r{std::span<std::byte>(buf)};

        stx::u32 vals[] = {0x1111, 0x2222, 0x3333};
        r.push(std::span<const stx::u32>(vals));
        REQUIRE(r.tell() == stx::off_s{12});

        r.seek(stx::off_s{0});
        REQUIRE(r.pop<stx::u32>() == 0x1111);
    }

    SECTION("push std::array")
    {
        std::array<std::byte, 8> buf{};
        stx::memcur r{std::span<std::byte>(buf)};

        std::array<stx::u32, 2> arr = {42, 99};
        r.push(arr);
        REQUIRE(r.tell() == stx::off_s{8});

        r.seek(stx::off_s{0});
        REQUIRE(r.pop<stx::u32>() == 42);
    }

    SECTION("read_into / pop_into")
    {
        std::array<std::byte, 16> buf{};
        stx::memcur r{std::span<std::byte>(buf)};

        stx::u32 vals[] = {1, 2, 3, 4};
        std::memcpy(buf.data(), vals, sizeof(vals));

        std::array<stx::u32, 2> dst{};
        r.read_into(dst);
        REQUIRE(dst[0] == 1);
        REQUIRE(dst[1] == 2);
        REQUIRE(r.tell() == stx::off_s{0});

        std::array<stx::u32, 2> dst2{};
        r.pop_into(dst2);
        REQUIRE(dst2[0] == 1);
        REQUIRE(dst2[1] == 2);
        REQUIRE(r.tell() == stx::off_s{8});
    }
}

TEST_CASE("Fs: map_file new API")
{
    SECTION("pop<T[N]> array")
    {
        auto path = std::filesystem::temp_directory_path() / "stx_test_map_new_pop.bin";
        std::error_code ec;

        {
            std::ofstream ofs(path, std::ios::binary);
            REQUIRE(ofs);
            stx::u32 data[] = {10, 20, 30, 40};
            ofs.write(reinterpret_cast<const char*>(data), sizeof(data));
        }

        auto mapped = stx::map_file::open(path);
        REQUIRE(mapped);
        auto& m = *mapped;

        auto arr = m.pop<stx::u32[4]>();
        REQUIRE(arr[0] == 10);
        REQUIRE(arr[3] == 40);
        REQUIRE(m.tell() == stx::off_s{16});

        std::filesystem::remove(path, ec);
    }

    SECTION("as_view no advance")
    {
        auto path = std::filesystem::temp_directory_path() / "stx_test_map_new_view.bin";
        std::error_code ec;

        {
            std::ofstream ofs(path, std::ios::binary);
            REQUIRE(ofs);
            stx::u32 data[] = {5, 6, 7};
            ofs.write(reinterpret_cast<const char*>(data), sizeof(data));
        }

        auto mapped = stx::map_file::open(path);
        REQUIRE(mapped);
        auto& m = *mapped;

        auto view = m.as_view<stx::u32[3]>();
        REQUIRE(view.size() == 3);
        REQUIRE(view[1] == 6);
        REQUIRE(m.tell() == stx::off_s{0});

        std::filesystem::remove(path, ec);
    }

    SECTION("push generic range")
    {
        auto path = std::filesystem::temp_directory_path() / "stx_test_map_new_push.bin";
        std::error_code ec;

        {
            std::ofstream ofs(path, std::ios::binary);
            REQUIRE(ofs);
            std::byte zero[16]{};
            ofs.write(reinterpret_cast<const char*>(zero), 16);
        }

        auto mapped = stx::map_file::open(path, stx::map_flag::write);
        REQUIRE(mapped);
        auto& m = *mapped;

        std::array<stx::u32, 3> arr = {100, 200, 300};
        m.push(arr);
        REQUIRE(m.tell() == stx::off_s{12});

        m.seek(stx::off_s{0});
        REQUIRE(m.pop<stx::u32>() == 100);

        std::filesystem::remove(path, ec);
    }
}

TEST_CASE("Fs: map_file flush")
{
    SECTION("flush persists data to disk")
    {
        auto path = std::filesystem::temp_directory_path() / "stx_test_map_flush.bin";
        std::error_code ec;

        {
            std::ofstream ofs(path, std::ios::binary);
            REQUIRE(ofs);
            stx::u32 data = 0x12345678;
            ofs.write(reinterpret_cast<const char*>(&data), sizeof(data));
        }

        {
            auto mapped = stx::map_file::open(path, stx::map_flag::write);
            REQUIRE(mapped);
            auto& m = *mapped;

            (m.as_p<stx::u32>() + stx::off_s{0}).write(stx::u32{0xDEADBEEF});

            auto result = m.flush();
            REQUIRE(result);
        }

        {
            auto mapped = stx::map_file::open(path);
            REQUIRE(mapped);
            auto& m = *mapped;

            stx::u32 val = (m.as_p<stx::u32>() + stx::off_s{0}).read<stx::u32>();
            REQUIRE(val == 0xDEADBEEF);
        }

        std::filesystem::remove(path, ec);
    }
}

TEST_CASE("Fs: map_file flags")
{
    SECTION("flags reflects write flag")
    {
        auto path = std::filesystem::temp_directory_path() / "stx_test_map_flags.bin";
        std::error_code ec;

        {
            std::ofstream ofs(path, std::ios::binary);
            REQUIRE(ofs);
            stx::u32 data = 0;
            ofs.write(reinterpret_cast<const char*>(&data), sizeof(data));
        }

        auto mapped = stx::map_file::open(path, stx::map_flag::write);
        REQUIRE(mapped);
        auto& m = *mapped;

        REQUIRE(!!(m.flags() & stx::map_flag::write));

        std::filesystem::remove(path, ec);
    }
}

TEST_CASE("Fs: map_file swap")
{
    SECTION("swap exchanges base addresses")
    {
        auto path1 = std::filesystem::temp_directory_path() / "stx_test_map_swp1.bin";
        auto path2 = std::filesystem::temp_directory_path() / "stx_test_map_swp2.bin";
        std::error_code ec;

        {
            std::ofstream ofs(path1, std::ios::binary);
            REQUIRE(ofs);
            stx::u32 data = 0x11111111;
            ofs.write(reinterpret_cast<const char*>(&data), sizeof(data));
        }
        {
            std::ofstream ofs(path2, std::ios::binary);
            REQUIRE(ofs);
            stx::u32 data = 0x22222222;
            ofs.write(reinterpret_cast<const char*>(&data), sizeof(data));
        }

        auto m1 = stx::map_file::open(path1);
        REQUIRE(m1);
        auto m2 = stx::map_file::open(path2);
        REQUIRE(m2);

        auto base1 = m1->base();
        auto base2 = m2->base();
        REQUIRE(base1 != base2);

        m1->swap(*m2);

        REQUIRE(m1->base() == base2);
        REQUIRE(m2->base() == base1);

        std::filesystem::remove(path1, ec);
        std::filesystem::remove(path2, ec);
    }
}

TEST_CASE("Fs: memcur advance")
{
    SECTION("advance moves tell() correctly")
    {
        stx::u32 data[] = {1, 2, 3, 4};
        stx::memcur r(data, sizeof(data));

        REQUIRE(r.tell() == stx::off_s{0});

        r.advance(stx::off_s{4});
        REQUIRE(r.tell() == stx::off_s{4});

        r.advance(stx::off_s{8});
        REQUIRE(r.tell() == stx::off_s{12});

        r.advance(stx::off_s{-4});
        REQUIRE(r.tell() == stx::off_s{8});
    }
}

TEST_CASE("Fs: memcur ptr constructor")
{
    SECTION("construct from ptr<ByteType> and size")
    {
        std::array<std::byte, 8> buf{};
        buf[0] = std::byte{0xAB};
        buf[7] = std::byte{0xCD};

        stx::ptr<std::byte> p{buf.data()};
        stx::memcur r{p, stx::usize{8}};

        REQUIRE(r);
        REQUIRE(r.size() == 8);
        REQUIRE(r.tell() == stx::off_s{0});

        REQUIRE(r.pop<stx::u8>() == 0xAB);
        REQUIRE(r.tell() == stx::off_s{1});
    }
}

TEST_CASE("Fs: read/write with span of bytes")
{
    SECTION("write then read back via span")
    {
        std::array<std::byte, 8> buf{};

        stx::mem::write(std::span<std::byte>(buf), stx::off_s{2}, stx::u32{0x12345678});

        auto rd = read<stx::u32>(std::span<const std::byte>(buf), stx::off_s{2});
        REQUIRE(rd);
        REQUIRE(*rd == 0x12345678);
    }
}
