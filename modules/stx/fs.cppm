module;

#include "lbyte/stx/fs.hpp"

export module lbyte.stx.fs;

import lbyte.stx.core;
import lbyte.stx.mem;

export namespace lbyte::stx
{
    using ::lbyte::stx::map_flag;
    using ::lbyte::stx::map_file;
    using ::lbyte::stx::memcur;
}

export namespace lbyte::stx::fs
{
    using ::lbyte::stx::fs::dirty_vector;
    using ::lbyte::stx::fs::origin;

    using ::lbyte::stx::fs::setpos;
    using ::lbyte::stx::fs::read;
    using ::lbyte::stx::fs::advance;

    using ::lbyte::stx::fs::write;
}

