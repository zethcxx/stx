module;

#include "lbyte/stx/bit.hpp"

export module lbyte.stx.bit;

export namespace lbyte::stx
{
    using ::lbyte::stx::bit_extract;
    using ::lbyte::stx::bit_mask;
    using ::lbyte::stx::bit_insert;
    using ::lbyte::stx::bit_test;
    using ::lbyte::stx::bit_set;
    using ::lbyte::stx::bit_clear;
    using ::lbyte::stx::bit_flip;

    using ::lbyte::stx::byte_extract;
    using ::lbyte::stx::byte_insert;
    using ::lbyte::stx::byte_mask;
    using ::lbyte::stx::byte_swap;
}

