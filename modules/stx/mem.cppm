module;

#include "lbyte/stx/mem.hpp"

export module lbyte.stx.mem;

import lbyte.stx.core;

export namespace lbyte::stx
{
    using ::lbyte::stx::rcast;
    using ::lbyte::stx::scast;
    using ::lbyte::stx::bcast;
    using ::lbyte::stx::ccast;

    using ::lbyte::stx::read;
    using ::lbyte::stx::read_raw;

    using ::lbyte::stx::write;
    using ::lbyte::stx::write_raw;

    using ::lbyte::stx::align_up;
    using ::lbyte::stx::align_down;
}
