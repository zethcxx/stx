#pragma once
#include "core.hpp"
#include <array>
#include <cstring>
#include <string_view>
#include <print>

#if defined(__GNUC__) || defined(__clang__)
    #define STX_FORCE_INLINE [[gnu::always_inline]] inline
#else
    #define STX_FORCE_INLINE inline
#endif

namespace stx
{
    template<class Type>
    [[nodiscard]] STX_FORCE_INLINE
    Type read( address_like auto base, usize off = 0 ) noexcept {
        return *rcast<Type*>(
            rcast<std::byte*>( addr_of( base )) + off
        );
    }

    template< class Type>
    STX_FORCE_INLINE
    void write( address_like auto base, usize off, Type value ) noexcept {
        *rcast<Type*>(
            rcast<std::byte*>( addr_of( base )) + off
        ) = value;
    }

    template<class Type>
    STX_FORCE_INLINE
    Type rcast( auto value ) noexcept {
        return reinterpret_cast<Type>( value );
    }

    template<class Type>
    STX_FORCE_INLINE
    constexpr Type scast( auto value ) noexcept {
        return static_cast<Type>( value );
    }

    template<typename To, typename From>
    [[nodiscard]] STX_FORCE_INLINE
    constexpr To bcast( const From& from ) noexcept {
        return std::bit_cast<To>( from );
    }

    template<std::unsigned_integral T>
    [[nodiscard]] STX_FORCE_INLINE
    constexpr T align_up( T value, T alignment ) noexcept {
        return ( value + alignment - 1 ) & ~( alignment - 1 );
    }

    template<std::unsigned_integral T>
    [[nodiscard]] STX_FORCE_INLINE
    constexpr T align_down( T value, T alignment ) noexcept {
        return value & ~( alignment - 1 );
    }

    [[gnu::no_sanitize("address")]]
    void dump( address_like auto base, usize size ) noexcept;
}

#undef STX_FORCE_INLINE


// NOLINTBEGIN(*)
void stx::dump( address_like auto base, usize limit ) noexcept
{
    constexpr u8 HEX_CHARS[]{ "0123456789abcdef" };

    constexpr auto hex_byte =
    [][[nodiscard]]( u8 *out, u8 value ) noexcept {
        *out++ = HEX_CHARS[value >> 4];
        *out++ = HEX_CHARS[value & 0xF];
        return out;
    };

    constexpr auto hex_ptr =
    [][[nodiscard]]( u8 *out, uptr value ) noexcept {
        static constexpr u32 WIDTH = sizeof( uptr ) * 2;

        for (u32 i = WIDTH - 1; i >= 0; --i)
        {
            out[i] = HEX_CHARS[value & 0xF];
            value >>= 4;
        }
        return out + WIDTH;
    };

	/** Number of hex digits required to represent a pointer:

		- 8 digits on 32-bit
		- 16 digits on 64-bit
	*/
    static constexpr u32 PTR_HEX_WIDTH  { sizeof( uptr ) * 2};
    static constexpr u32 BYTES_PER_LINE { 16 };

    /** Buffer layout per line:

		  16          : color escape sequences
		+ 2           : prefix "0x"
		+ 2           : separator ": "
		+ 3*16        : hex bytes + spaces
		+ 16          : ASCII dump
		84 = PADDING
	*/
    static thread_local std::array<u8, PTR_HEX_WIDTH + 84> buff_line;

    const auto *mem { rcast<const u8*>( base ) };

    static constexpr std::string_view COLOR_ADDR  { "\x1b[38;5;12m" };
    static constexpr std::string_view COLOR_RESET { "\x1b[0m"       };

    for ( usize offset {}; offset < limit; offset += BYTES_PER_LINE )
    {
        auto *iter { buff_line.data() };

        // add color for the address
        std::memcpy( iter, COLOR_ADDR.data(), COLOR_ADDR.size() );

        // 0x + addr
        *iter++ = '0';
        *iter++ = 'x';

        const auto addr { rcast<uptr>( mem ) + offset };
        iter = hex_ptr( iter, addr );

        const auto line_bytes { std::min<usize>( BYTES_PER_LINE, limit - offset ) };

        // reset color
        std::memcpy( iter, COLOR_RESET.data(), COLOR_RESET.size());
        iter += COLOR_RESET.size();

        *iter++ = ':';
        *iter++ = ' ';

        for ( usize idx {}; idx < line_bytes; idx++ )
        {
            iter = hex_byte( iter, mem[offset + idx] );
            *iter++ = ' ';
        }

        // extra padding if missing bytes
        if ( line_bytes < BYTES_PER_LINE )
        {
            const usize padding { BYTES_PER_LINE - line_bytes * 3 };
            std::memset( iter, ' ', padding );
            iter += padding;
        }

        // string column
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
// NOLINTEND(*)
