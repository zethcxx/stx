module;

#include "lbyte/stx/endian.hpp"

export module lbyte.stx.endian;

export namespace lbyte::stx
{
    using ::lbyte::stx::byte_order;
    using ::lbyte::stx::endian_compatible;
    using ::lbyte::stx::endian_value;
    using ::lbyte::stx::le;
    using ::lbyte::stx::be;
    using ::lbyte::stx::is_endian_value_v;
}

