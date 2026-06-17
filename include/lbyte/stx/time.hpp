#pragma once

#include "../stx/core.hpp"
#include <chrono>

namespace lbyte::stx::time
{
    using namespace lbyte::stx;
    // CLOCK ALIASES -------------------------------------------------------------
    using wall_clock   = std::chrono::system_clock;
    using hires_clock  = std::chrono::high_resolution_clock;
    using steady_clock = std::chrono::steady_clock;

    // UNIX TIME UTILITIES ------------------------------------------------------
    template<typename Dur = std::chrono::seconds> [[nodiscard]]
    constexpr wall_clock::time_point from_unix(u64 count) noexcept
    {
        return wall_clock::time_point{ Dur{count} };
    }

    template<typename Dur = std::chrono::seconds> [[nodiscard]]
    constexpr u64 to_unix(wall_clock::time_point tp) noexcept
    {
        return static_cast<u64>(
            std::chrono::duration_cast<Dur>(tp.time_since_epoch()).count()
        );
    }

    template<typename Dur = std::chrono::seconds> [[nodiscard]]
    inline u64 now() noexcept
    {
        return to_unix<Dur>(wall_clock::now());
    }

    [[nodiscard]] inline u64 now_ms() noexcept
    {
        return now<std::chrono::milliseconds>();
    }

    [[nodiscard]] inline u64 now_ns() noexcept
    {
        return now<std::chrono::nanoseconds>();
    }

    // STOPWATCH -----------------------------------------------------------------
    struct stopwatch
    {
        steady_clock::time_point start_{ steady_clock::now() };

        template<class Duration = std::chrono::milliseconds>
        [[nodiscard]] Duration elapsed() const noexcept
        {
            return std::chrono::duration_cast<Duration>(
                steady_clock::now() - start_
            );
        }

        template<class Duration = std::chrono::milliseconds>
        [[nodiscard]] Duration lap() noexcept
        {
            auto prev = start_;
            start_ = steady_clock::now();
            return std::chrono::duration_cast<Duration>(start_ - prev);
        }

        void reset() noexcept
        {
            start_ = steady_clock::now();
        }
    };

    // PORTABLE BINARY FORMAT CONVERTERS ----------------------------------------

    // --- FILETIME (Windows) ----------------------------------------------------
    [[nodiscard]] constexpr wall_clock::time_point from_filetime(u64 ft) noexcept
    {
        constexpr u64 epoch_ft = 116'444'736'000'000'000ULL;
        u64 sec = (ft >= epoch_ft) ? ((ft - epoch_ft) / 10'000'000) : 0;
        return wall_clock::time_point{ std::chrono::seconds{sec} };
    }

    [[nodiscard]] constexpr u64 to_filetime(wall_clock::time_point tp) noexcept
    {
        constexpr i64 epoch_sec = 116'444'736'00LL;
        auto dur = tp.time_since_epoch();
        auto sec = std::chrono::duration_cast<std::chrono::seconds>(dur).count();
        return static_cast<u64>(sec + epoch_sec) * 10'000'000;
    }

    // --- DOS date/time ---------------------------------------------------------
    [[nodiscard]] constexpr wall_clock::time_point from_dos(u32 dos) noexcept
    {
        using namespace std::chrono;

        u16 date = static_cast<u16>(dos >> 16);
        u16 time = static_cast<u16>(dos & 0xFFFF);

        int y = ((date >> 9) & 0x7F) + 1980;
        int m = (date >> 5) & 0x0F;
        int d = date & 0x1F;

        int hh = (time >> 11) & 0x1F;
        int mm = (time >> 5)  & 0x3F;
        int ss = (time & 0x1F) * 2;

        if (y < 1980 || y > 2107 || m < 1 || m > 12 || d < 1 || d > 31 ||
            hh > 23 || mm > 59 || ss > 59)
            return wall_clock::time_point{};

        sys_days dp = year{y} / month{static_cast<unsigned>(m)} / day{static_cast<unsigned>(d)};
        return wall_clock::time_point{ dp.time_since_epoch() + hours{hh} + minutes{mm} + seconds{ss} };
    }

    [[nodiscard]] constexpr u32 to_dos(wall_clock::time_point tp) noexcept
    {
        using namespace std::chrono;

        auto dp = floor<days>(tp);
        year_month_day ymd{dp};
        auto tod = hh_mm_ss{tp - sys_days{dp}};

        int y = int{ymd.year()};
        unsigned m = unsigned{ymd.month()};
        unsigned d = unsigned{ymd.day()};

        auto h = tod.hours().count();
        auto mi = tod.minutes().count();
        auto s = tod.seconds().count();

        if (y < 1980 || y > 2107) return 0;

        u32 date = (static_cast<u32>(y - 1980) << 9) | (m << 5) | d;
        u32 tim = (static_cast<u32>(h) << 11) | (static_cast<u32>(mi) << 5) | static_cast<u32>(s / 2);
        return (date << 16) | tim;
    }

    // --- NTP timestamp ---------------------------------------------------------
    [[nodiscard]] constexpr wall_clock::time_point from_ntp(u32 seconds) noexcept
    {
        constexpr u64 epoch_ntp = 2'208'988'800ULL;
        u64 s = (static_cast<u64>(seconds) >= epoch_ntp) ? (static_cast<u64>(seconds) - epoch_ntp) : 0;
        return wall_clock::time_point{ std::chrono::seconds{s} };
    }

    [[nodiscard]] constexpr wall_clock::time_point from_ntp(u64 ntp_ts) noexcept
    {
        return from_ntp(static_cast<u32>(ntp_ts >> 32));
    }

    [[nodiscard]] constexpr u32 to_ntp(wall_clock::time_point tp) noexcept
    {
        constexpr u64 epoch_ntp = 2'208'988'800ULL;
        auto dur = tp.time_since_epoch();
        auto sec = std::chrono::duration_cast<std::chrono::seconds>(dur).count();
        return static_cast<u32>(static_cast<u64>(sec + static_cast<i64>(epoch_ntp)));
    }
}
