module;

#include "lbyte/stx/io.hpp"

export module lbyte.stx.io;

import lbyte.stx.core;
import lbyte.stx.mem;

export namespace lbyte::stx
{
    using ::lbyte::stx::map_flag;
    using ::lbyte::stx::map_file;
    using ::lbyte::stx::memcur;
}

export namespace lbyte::stx::io
{
    using ::lbyte::stx::io::dirty_vector;
    using ::lbyte::stx::io::origin;

    using ::lbyte::stx::io::setpos;
    using ::lbyte::stx::io::read;
    using ::lbyte::stx::io::advance;

    using ::lbyte::stx::io::write;
}

