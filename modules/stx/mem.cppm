module;

#define LBYTE_STX_MODULE
#include "lbyte/stx/mem.hpp"

export module lbyte.stx.mem;

import lbyte.stx.core;

export namespace lbyte::stx
{
    using ::lbyte::stx::rcast;
    using ::lbyte::stx::scast;
    using ::lbyte::stx::bcast;
    using ::lbyte::stx::ccast;
    using ::lbyte::stx::dcast;

    using ::lbyte::stx::read;
    using ::lbyte::stx::read_raw;
    using ::lbyte::stx::read_le;
    using ::lbyte::stx::read_be;

    using ::lbyte::stx::write;
    using ::lbyte::stx::write_raw;
    using ::lbyte::stx::write_le;
    using ::lbyte::stx::write_be;

    using ::lbyte::stx::ptr;

    using ::lbyte::stx::align_up;
    using ::lbyte::stx::align_down;
}

export template<typename T, lbyte::stx::uptr Stride>
struct std::hash<lbyte::stx::ptr<T, Stride>>
{
    auto operator()( const lbyte::stx::ptr<T, Stride>& p ) const noexcept {
        return std::hash<lbyte::stx::uptr>{}( p.addr() );
    }
};
