#pragma once
#include "./core.hpp"
#include <chrono>

namespace stx
{
    template<class Duration = std::chrono::seconds>
    inline std::chrono::time_point<std::chrono::system_clock, Duration>
    to_time( u64 unix ) noexcept
    {
        return std::chrono::time_point<
            std::chrono::system_clock,
            Duration
        >{ Duration{ unix } };
    }
}
