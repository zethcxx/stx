module;

#include "lbyte/stx/fs.hpp"

export module lbyte.stx.fs;

import lbyte.stx.core;
import lbyte.stx.mem;

export namespace lbyte::stx
{
    using ::lbyte::stx::dirty_vector;

    using ::lbyte::stx::map_flag;
    using ::lbyte::stx::map_file;
    using ::lbyte::stx::memcur;
}

export namespace lbyte::stx::fs
{
    using ::lbyte::stx::fs::origin;

    using ::lbyte::stx::fs::setposfs;
    using ::lbyte::stx::fs::readfs;
    using ::lbyte::stx::fs::skipfs;

    using ::lbyte::stx::fs::setposos;
    using ::lbyte::stx::fs::writefs;
    using ::lbyte::stx::fs::skipos;
}

