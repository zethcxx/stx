module;

#include "lbyte/stx/literals.hpp"

export module lbyte.stx.literals;

import lbyte.stx.core;
import lbyte.stx.mem;
import lbyte.stx.endian;

export namespace lbyte::stx::literals
{
    using ::lbyte::stx::literals::operator""_f32;
    using ::lbyte::stx::literals::operator""_f64;

    using ::lbyte::stx::literals::operator""_u8;
    using ::lbyte::stx::literals::operator""_u16;
    using ::lbyte::stx::literals::operator""_u32;
    using ::lbyte::stx::literals::operator""_u64;

    using ::lbyte::stx::literals::operator""_i8;
    using ::lbyte::stx::literals::operator""_i16;
    using ::lbyte::stx::literals::operator""_i32;
    using ::lbyte::stx::literals::operator""_i64;

    using ::lbyte::stx::literals::operator""_uz;

    using ::lbyte::stx::literals::operator""_off_s;
    using ::lbyte::stx::literals::operator""_rva_s;
    using ::lbyte::stx::literals::operator""_va_s;

    using ::lbyte::stx::literals::operator""_ptr;

    using ::lbyte::stx::literals::operator""_kb;
    using ::lbyte::stx::literals::operator""_mb;
    using ::lbyte::stx::literals::operator""_gb;

    using ::lbyte::stx::literals::operator""_le;
    using ::lbyte::stx::literals::operator""_be;
}

