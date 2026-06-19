module;

#include "lbyte/stx/ct.hpp"

export module lbyte.stx.ct;

import lbyte.stx.core;

export namespace lbyte::stx::ct
{
    using ::lbyte::stx::ct::fixed_string;
    using ::lbyte::stx::ct::fmt;
    using ::lbyte::stx::ct::endian;
    using ::lbyte::stx::ct::str_type;
    using ::lbyte::stx::ct::str;
    using ::lbyte::stx::ct::byte_block;
    using ::lbyte::stx::ct::args;
    using ::lbyte::stx::ct::formatter;
    using ::lbyte::stx::ct::istr_t;
    using ::lbyte::stx::ct::istr;
    using ::lbyte::stx::ct::vstr;

#if __has_include(<ctre.hpp>)
    using ::lbyte::stx::ct::re;
#endif
}
