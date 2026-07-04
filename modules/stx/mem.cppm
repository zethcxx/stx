module;

#define STX_MODULE_BUILD
#include "lbyte/stx/mem.hpp"

export module lbyte.stx.mem;

import lbyte.stx.core;

export namespace lbyte::stx
{
    using ::lbyte::stx::ptr;
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

    using ::lbyte::stx::mem::diff;

    using ::lbyte::stx::mem::align_up;
    using ::lbyte::stx::mem::align_down;
    using ::lbyte::stx::mem::gap_align_v;
    using ::lbyte::stx::mem::gap_v;
}

// --- std::hash --------------------------------------------------------------------

template<typename T>
struct std::hash<lbyte::stx::ptr<T>>
{
    [[nodiscard]] auto operator()( const lbyte::stx::ptr<T>& p ) const noexcept {
        return std::hash<lbyte::stx::uptr>{}( p.addr() );
    }
};

// --- std::formatter ---------------------------------------------------------------

#include <format>

template<typename T>
struct std::formatter<lbyte::stx::ptr<T>> : std::formatter<void*> {
    auto format(const lbyte::stx::ptr<T>& p, format_context& ctx) const {
        return std::formatter<void*>::format(reinterpret_cast<void*>(p.addr()), ctx);
    }
};
