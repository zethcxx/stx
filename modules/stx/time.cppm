module;

#include "lbyte/stx/time.hpp"

export module lbyte.stx.time;

import lbyte.stx.core;

export namespace lbyte::stx::time
{
    using ::lbyte::stx::time::wall_clock;
    using ::lbyte::stx::time::hires_clock;
    using ::lbyte::stx::time::steady_clock;

    using ::lbyte::stx::time::from_unix;
    using ::lbyte::stx::time::to_unix;
    using ::lbyte::stx::time::now;
    using ::lbyte::stx::time::now_ms;
    using ::lbyte::stx::time::now_ns;

    using ::lbyte::stx::time::stopwatch;

    using ::lbyte::stx::time::from_filetime;
    using ::lbyte::stx::time::to_filetime;
    using ::lbyte::stx::time::from_dos;
    using ::lbyte::stx::time::to_dos;
    using ::lbyte::stx::time::from_ntp;
    using ::lbyte::stx::time::to_ntp;
}

