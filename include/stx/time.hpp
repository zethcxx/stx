#pragma once

#include "./core.hpp"
#include <chrono>

namespace stx
{
    using sys_clock = std::chrono::system_clock;
    using hr_clock  = std::chrono::high_resolution_clock;

    // -------------------------------------------------------------------------
    // UNIX TIME UTILITIES (UTC)
    // Epoch: 1970-01-01 00:00:00 UTC
    // -------------------------------------------------------------------------

    [[nodiscard]] inline constexpr sys_clock::time_point
    from_unix_seconds(u64 seconds) noexcept
    {
        return sys_clock::time_point{
            std::chrono::seconds{ seconds }
        };
    }

    [[nodiscard]] inline constexpr sys_clock::time_point
    from_unix_millis(u64 millis) noexcept
    {
        return sys_clock::time_point{
            std::chrono::milliseconds{ millis }
        };
    }

    [[nodiscard]] inline constexpr u64
    to_unix_seconds(sys_clock::time_point tp) noexcept
    {
        return static_cast<u64>(
            std::chrono::duration_cast<std::chrono::seconds>(
                tp.time_since_epoch()
            ).count()
        );
    }

    [[nodiscard]] inline constexpr u64
    to_unix_millis(sys_clock::time_point tp) noexcept
    {
        return static_cast<u64>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                tp.time_since_epoch()
            ).count()
        );
    }

    [[nodiscard]] inline u64
    unix_seconds_now() noexcept
    {
        return to_unix_seconds(sys_clock::now());
    }

    [[nodiscard]] inline u64
    unix_millis_now() noexcept
    {
        return to_unix_millis(sys_clock::now());
    }

    // -------------------------------------------------------------------------
    // STOP WATCH (High Resolution Clock)
    // -------------------------------------------------------------------------

    struct stop_watch
    {
        hr_clock::time_point start_point{ hr_clock::now() };

        template<class Duration = std::chrono::milliseconds>
        [[nodiscard]] inline u64 elapsed() const noexcept
        {
            return scast<u64>(
                std::chrono::duration_cast<Duration>(
                    hr_clock::now() - start_point
                ).count()
            );
        }

        inline void reset() noexcept
        {
            start_point = hr_clock::now();
        }
    };
}
