module;

#include "lbyte/stx/endian.hpp"

export module lbyte.stx.endian;

export namespace lbyte::stx::endian
{
    using ::lbyte::stx::endian::order;
    using ::lbyte::stx::endian::compatible;
    using ::lbyte::stx::endian::endian_value;
    using ::lbyte::stx::endian::le;
    using ::lbyte::stx::endian::be;
    using ::lbyte::stx::endian::is_endian_value_v;
}
