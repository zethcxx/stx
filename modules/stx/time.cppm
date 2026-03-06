module;

#include "lbyte/stx/time.hpp"

export module lbyte.stx.time;

import lbyte.stx.core;

export namespace lbyte::stx
{
    using ::lbyte::stx::sys_clock;
    using ::lbyte::stx::hr_clock;

    using ::lbyte::stx::from_unix_seconds;
    using ::lbyte::stx::from_unix_millis;
    using ::lbyte::stx::to_unix_seconds;
    using ::lbyte::stx::to_unix_millis;
    using ::lbyte::stx::unix_seconds_now;
    using ::lbyte::stx::unix_millis_now;

    using ::lbyte::stx::stop_watch;
}
