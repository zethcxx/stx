#include <catch2/catch_test_macros.hpp>
#include <lbyte/stx/time.hpp>
#include <lbyte/stx/mem.hpp>

#include <chrono>

using namespace lbyte;
#include <thread>

TEST_CASE("Time: Clock Aliases")
{
    SECTION("sys_clock is std::chrono::system_clock")
    {
        static_assert(std::is_same_v<stx::sys_clock, std::chrono::system_clock>);
    }

    SECTION("hr_clock is std::chrono::high_resolution_clock")
    {
        static_assert(std::is_same_v<stx::hr_clock, std::chrono::high_resolution_clock>);
    }
}

TEST_CASE("Time: Unix Time Conversion")
{
    SECTION("from_unix_seconds creates correct time_point")
    {
        constexpr auto tp = stx::from_unix_seconds(0);
        auto epoch = std::chrono::system_clock::from_time_t(0);

        auto diff = std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch()).count();
        REQUIRE(diff == 0);
    }

    SECTION("from_unix_millis creates correct time_point")
    {
        constexpr auto tp = stx::from_unix_millis(1000);
        auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count();
        REQUIRE(diff == 1000);
    }

    SECTION("to_unix_seconds extracts correct value")
    {
        auto tp = stx::sys_clock::from_time_t(12345);
        REQUIRE(stx::to_unix_seconds(tp) == 12345);
    }

    SECTION("to_unix_millis extracts correct value")
    {
        auto tp = std::chrono::time_point<stx::sys_clock>(std::chrono::milliseconds{12345678});
        REQUIRE(stx::to_unix_millis(tp) == 12345678);
    }
}

TEST_CASE("Time: Round-trip conversion")
{
    SECTION("from_unix_seconds round-trip")
    {
        stx::u64 original = 1609459200;
        auto tp = stx::from_unix_seconds(original);
        stx::u64 recovered = stx::to_unix_seconds(tp);
        REQUIRE(recovered == original);
    }

    SECTION("from_unix_millis round-trip")
    {
        stx::u64 original = 1609459200000;
        auto tp = stx::from_unix_millis(original);
        stx::u64 recovered = stx::to_unix_millis(tp);
        REQUIRE(recovered == original);
    }
}

TEST_CASE("Time: unix_seconds_now and unix_millis_now")
{
    SECTION("unix_seconds_now returns reasonable value")
    {
        stx::u64 now_seconds = stx::unix_seconds_now();
        REQUIRE(now_seconds > 1609459200);
    }

    SECTION("unix_millis_now returns reasonable value")
    {
        stx::u64 now_millis = stx::unix_millis_now();
        REQUIRE(now_millis > 1609459200000);
    }

    SECTION("millis is approximately 1000x seconds")
    {
        stx::u64 seconds = stx::unix_seconds_now();
        stx::u64 millis = stx::unix_millis_now();

        REQUIRE(millis / 1000 == seconds);
    }
}

TEST_CASE("Time: Stop Watch")
{
    SECTION("stop_watch measures elapsed time")
    {
        stx::stop_watch sw;

        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        stx::u64 elapsed_ms = sw.elapsed<std::chrono::milliseconds>();
        REQUIRE(elapsed_ms >= 40);
    }

    SECTION("stop_watch reset restarts timer")
    {
        stx::stop_watch sw;

        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        sw.reset();

        std::this_thread::sleep_for(std::chrono::milliseconds(30));

        stx::u64 elapsed_ms = sw.elapsed<std::chrono::milliseconds>();
        REQUIRE(elapsed_ms < 50);
    }

    SECTION("stop_watch with different duration types")
    {
        stx::stop_watch sw;

        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        stx::u64 elapsed_ms = sw.elapsed<std::chrono::milliseconds>();
        stx::u64 elapsed_us = sw.elapsed<std::chrono::microseconds>();

        REQUIRE(elapsed_us > elapsed_ms * 900);
        REQUIRE(elapsed_us < elapsed_ms * 1100);
    }

    SECTION("stop_watch initial elapsed is zero or near-zero")
    {
        stx::stop_watch sw;
        stx::u64 elapsed = sw.elapsed<std::chrono::milliseconds>();
        REQUIRE(elapsed < 10);
    }
}

TEST_CASE("Time: Compile-time constants")
{
    SECTION("time_point is constructible at compile time")
    {
        constexpr auto tp = stx::from_unix_seconds(100);
        constexpr stx::u64 seconds = stx::to_unix_seconds(tp);
        static_assert(seconds == 100);
    }
}
