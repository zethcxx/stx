module;

// Global module fragment
#include "lbyte/stx/fs.hpp"

export module lbyte.stx.fs;

import lbyte.stx.core;
import lbyte.stx.mem;

export namespace lbyte::stx
{
    using ::lbyte::stx::dirty_vector;
    using ::lbyte::stx::setposfs;
    using ::lbyte::stx::readfs;
    using ::lbyte::stx::skipfs;

    using ::lbyte::stx::setposos;
    using ::lbyte::stx::writefs;
    using ::lbyte::stx::skipos;

    using ::lbyte::stx::map_flag;
    using ::lbyte::stx::map_file;
    using ::lbyte::stx::reader_view;
    using ::lbyte::stx::reader_raw;
}
