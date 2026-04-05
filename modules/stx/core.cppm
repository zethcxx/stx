module;

#include "lbyte/stx/core.hpp"

export module lbyte.stx.core;

export namespace lbyte::stx
{
    using ::lbyte::stx::u8;
    using ::lbyte::stx::u16;
    using ::lbyte::stx::u32;
    using ::lbyte::stx::u64;

    using ::lbyte::stx::i8;
    using ::lbyte::stx::i16;
    using ::lbyte::stx::i32;
    using ::lbyte::stx::i64;

    using ::lbyte::stx::f32;
    using ::lbyte::stx::f64;

    using ::lbyte::stx::isize;
    using ::lbyte::stx::usize;

    using ::lbyte::stx::uptr;
    using ::lbyte::stx::iptr;

    using ::lbyte::stx::version_info;
    using ::lbyte::stx::version;
    using ::lbyte::stx::origin;
    using ::lbyte::stx::off_s;
    using ::lbyte::stx::rva_s;
    using ::lbyte::stx::va_s;

    using ::lbyte::stx::address_like;
    using ::lbyte::stx::binary_readable;

    using ::lbyte::stx::normalize_addr;

    using ::lbyte::stx::gap_v;
    using ::lbyte::stx::gap_align_v;
}
