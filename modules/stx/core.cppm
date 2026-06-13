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

    using ::lbyte::stx::uchar;
    using ::lbyte::stx::ushort;
    using ::lbyte::stx::uint;
    using ::lbyte::stx::ulong;
    using ::lbyte::stx::ulonglong;

    using ::lbyte::stx::f32;
    using ::lbyte::stx::f64;

    using ::lbyte::stx::isize;
    using ::lbyte::stx::usize;

    using ::lbyte::stx::uptr;
    using ::lbyte::stx::iptr;

    using ::lbyte::stx::version_info;
    using ::lbyte::stx::version;
    using ::lbyte::stx::off_s;
    using ::lbyte::stx::rva_s;
    using ::lbyte::stx::va_s;

    using ::lbyte::stx::address_like;
    using ::lbyte::stx::binary_readable;
    using ::lbyte::stx::byte_swappable;
    using ::lbyte::stx::byte_offset;

    using ::lbyte::stx::normalize_addr;

    using ::lbyte::stx::rcast;
    using ::lbyte::stx::scast;
    using ::lbyte::stx::bcast;
    using ::lbyte::stx::ccast;
    using ::lbyte::stx::dcast;

    using ::lbyte::stx::contiguous_buffer;
    using ::lbyte::stx::writable_buffer;
    using ::lbyte::stx::bounded_array;
    using ::lbyte::stx::buffer_type;


    using ::lbyte::stx::defer;
    using ::lbyte::stx::null_t;
    using ::lbyte::stx::null;
}

