#pragma once

#include "./mem.hpp"
#include <array>
#include <print>
#include <string_view>

namespace lbyte::stx::details
{
    static constexpr u8 HEX_CHARS[]{ "0123456789abcdef" };

    [[gnu::always_inline]]
    inline u8* hex_byte( u8 *out, u8 value ) noexcept {
        *out++ = HEX_CHARS[value >> 4];
        *out++ = HEX_CHARS[value & 0xF];
        return out;
    }

    [[gnu::always_inline]]
    inline u8* hex_ptr( u8 *out, uptr value ) noexcept {
        static constexpr i32 WIDTH { sizeof( uptr ) * 2 };

        for (i32 i = WIDTH - 1; i >= 0; --i) {
            out[i] = HEX_CHARS[value & 0xF];
            value >>= 4;
        }

        return out + WIDTH;
    }
}

namespace lbyte::stx
{
    [[gnu::no_sanitize("address")]]
    void dump( address_like auto base, usize size ) noexcept;
}

void lbyte::stx::dump( address_like auto base, usize limit ) noexcept
{
    static constexpr u32 PTR_HEX_WIDTH  { sizeof( uptr ) * 2 };
    static constexpr u32 BYTES_PER_LINE { 16 };

    static thread_local std::array<u8, PTR_HEX_WIDTH + 84> buff_line;

    const auto *mem { rcast<const u8*>( base ) };

    static constexpr std::string_view COLOR_ADDR  { "\x1b[38;5;12m" };
    static constexpr std::string_view COLOR_RESET { "\x1b[0m"       };

    for ( usize offset {}; offset < limit; offset += BYTES_PER_LINE )
    {
        auto *iter { buff_line.data() };

        std::memcpy( iter, COLOR_ADDR.data(), COLOR_ADDR.size() );
        iter += COLOR_ADDR.size();

        *iter++ = '0';
        *iter++ = 'x';

        const auto addr { rcast<uptr>( mem ) + offset };
        iter = details::hex_ptr( iter, addr );

        std::memcpy( iter, COLOR_RESET.data(), COLOR_RESET.size());
        iter += COLOR_RESET.size();

        *iter++ = ':';
        *iter++ = ' ';

        const auto line_bytes { std::min<usize>( BYTES_PER_LINE, limit - offset ) };

        for ( usize idx {}; idx < line_bytes; idx++ )
        {
            iter = details::hex_byte( iter, mem[offset + idx] );
            *iter++ = ' ';
        }

        if ( line_bytes < BYTES_PER_LINE )
        {
            const usize padding { BYTES_PER_LINE - line_bytes * 3 };
            std::memset( iter, ' ', padding );
            iter += padding;
        }

        *iter++ = '|';

        for ( usize idx {}; idx < BYTES_PER_LINE; ++idx )
        {
            const u8 byte { mem[offset + idx] };

            if ( idx < line_bytes )
                *iter++ = scast<u8>(
                    ( byte - 32u < 95u )
                        ? byte
                        : '.'
                );
            else
                *iter++ = ' ';
        }

        *iter++ = '|';

        std::println( "{}", std::string_view {
                rcast<char*>( buff_line.data()       ),
                scast<usize>( iter - buff_line.data())
        });
    }
}
