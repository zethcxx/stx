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
    using ::lbyte::stx::dcast;

    using ::lbyte::stx::read;
    using ::lbyte::stx::read_raw;
    using ::lbyte::stx::read_le;
    using ::lbyte::stx::read_be;

    using ::lbyte::stx::write;
    using ::lbyte::stx::write_raw;
    using ::lbyte::stx::write_le;
    using ::lbyte::stx::write_be;

    using ::lbyte::stx::ptr;
    using ::lbyte::stx::wptr;

    using ::lbyte::stx::align_up;
    using ::lbyte::stx::align_down;
}
