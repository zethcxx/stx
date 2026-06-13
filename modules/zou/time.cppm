module;

#include "lbyte/zou/time.hpp"

export module lbyte.zou.time;

import lbyte.stx.core;

export namespace lbyte::zou::time
{
    using ::lbyte::zou::time::wall_clock;
    using ::lbyte::zou::time::hires_clock;
    using ::lbyte::zou::time::steady_clock;

    using ::lbyte::zou::time::from_unix;
    using ::lbyte::zou::time::to_unix;
    using ::lbyte::zou::time::now;
    using ::lbyte::zou::time::now_ms;
    using ::lbyte::zou::time::now_ns;

    using ::lbyte::zou::time::stopwatch;

    using ::lbyte::zou::time::from_filetime;
    using ::lbyte::zou::time::to_filetime;
    using ::lbyte::zou::time::from_dos;
    using ::lbyte::zou::time::to_dos;
    using ::lbyte::zou::time::from_ntp;
    using ::lbyte::zou::time::to_ntp;
}
