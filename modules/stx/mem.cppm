module;

#include "lbyte/stx/mem.hpp"

export module lbyte.stx.mem;

import lbyte.stx.core;

export namespace lbyte::stx
{
    using ::lbyte::stx::ptr;
    using ::lbyte::stx::ptr_light;
}

export namespace lbyte::stx::mem
{
    using ::lbyte::stx::mem::read;
    using ::lbyte::stx::mem::read_raw;
    using ::lbyte::stx::mem::read_le;
    using ::lbyte::stx::mem::read_be;

    using ::lbyte::stx::mem::write;
    using ::lbyte::stx::mem::write_raw;
    using ::lbyte::stx::mem::write_le;
    using ::lbyte::stx::mem::write_be;

    using ::lbyte::stx::mem::align_up;
    using ::lbyte::stx::mem::align_down;
    using ::lbyte::stx::mem::gap_align_v;
    using ::lbyte::stx::mem::gap_v;
}

