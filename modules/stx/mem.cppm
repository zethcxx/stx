module;

#define LBYTE_STX_MODULE
#include "lbyte/stx/mem.hpp"

export module lbyte.stx.mem;

import lbyte.stx.core;

export namespace lbyte::stx
{
    using ::lbyte::stx::ptr;

    using ::lbyte::stx::align_up;
    using ::lbyte::stx::align_down;
}

export namespace lbyte::stx::mem
{
    using ::lbyte::stx::mem::read;
    using ::lbyte::stx::mem::read_raw;
    using ::lbyte::stx::mem::read_le;
    using ::lbyte::stx::mem::read_be;

    using ::lbyte::stx::mem::write;
    using ::lbyte::stx::mem::write_raw;
    using ::lbyte::stx::mem::write_le;
    using ::lbyte::stx::mem::write_be;
}

export template<typename T, lbyte::stx::uptr Stride>
struct std::hash<lbyte::stx::ptr<T, Stride>>
{
    auto operator()( const lbyte::stx::ptr<T, Stride>& p ) const noexcept {
        return std::hash<lbyte::stx::uptr>{}( p.addr() );
    }
};
