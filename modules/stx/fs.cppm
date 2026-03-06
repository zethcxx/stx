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
    using ::lbyte::stx::last_read_ok;

}
