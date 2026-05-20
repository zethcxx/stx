#pragma once
#include "./mem.hpp"
#include "./endian.hpp"

namespace lbyte::stx::literals
{
    // --- FLOATING TYPES ------------------------------------------------------
    constexpr float  operator""_f32( long double        v ) noexcept { return static_cast<float >(v); }
    constexpr float  operator""_f32( unsigned long long v ) noexcept { return static_cast<float >(v); }
    constexpr double operator""_f64( long double        v ) noexcept { return static_cast<double>(v); }
    constexpr double operator""_f64( unsigned long long v ) noexcept { return static_cast<double>(v); }

    // --- INTEGER TYPES -------------------------------------------------------
    constexpr u8    operator""_u8  ( unsigned long long v ) noexcept { return static_cast<u8   >(v); }
    constexpr u16   operator""_u16 ( unsigned long long v ) noexcept { return static_cast<u16  >(v); }
    constexpr u32   operator""_u32 ( unsigned long long v ) noexcept { return static_cast<u32  >(v); }
    constexpr u64   operator""_u64 ( unsigned long long v ) noexcept { return static_cast<u64  >(v); }
    constexpr i8    operator""_i8  ( unsigned long long v ) noexcept { return static_cast<i8   >(v); }
    constexpr i16   operator""_i16 ( unsigned long long v ) noexcept { return static_cast<i16  >(v); }
    constexpr i32   operator""_i32 ( unsigned long long v ) noexcept { return static_cast<i32  >(v); }
    constexpr i64   operator""_i64 ( unsigned long long v ) noexcept { return static_cast<i64  >(v); }
    constexpr usize operator""_uz  ( unsigned long long v ) noexcept { return static_cast<usize>(v); }

    // --- STRONG TYPES --------------------------------------------------------
    constexpr off_s operator""_off_s( unsigned long long v ) noexcept {
        return off_s{ static_cast<off_s::value_type>(v) };
    }

    constexpr rva_s operator""_rva_s( unsigned long long v ) noexcept {
        return rva_s{ static_cast<rva_s::value_type>(v) };
    }

    constexpr va_s operator""_va_s( unsigned long long v ) noexcept {
        return va_s{ static_cast<va_s::value_type>(v) };
    }

    // --- SIZE LITERALS (powers of 1024) ------------------------------------
    constexpr usize operator""_kb( unsigned long long v ) noexcept {
        return static_cast<usize>( v ) * 1024uz;
    }

    constexpr usize operator""_mb( unsigned long long v ) noexcept {
        return static_cast<usize>( v ) * 1024uz * 1024uz;
    }

    constexpr usize operator""_gb( unsigned long long v ) noexcept {
        return static_cast<usize>( v ) * 1024uz * 1024uz * 1024uz;
    }

    // --- POINTERS (default to std::byte) ------------------------------------
    constexpr ptr<std::byte> operator""_ptr( unsigned long long v ) noexcept {
        return ptr<std::byte>{ static_cast<uptr>(v) };
    }

    // --- ENDIAN TYPES (auto-sized) -----------------------------------------
    namespace details {
        constexpr auto hex_val(char c) noexcept -> unsigned long long {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
            if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
            return 0;
        }

        constexpr auto pow10(unsigned long long n) noexcept -> unsigned long long {
            auto r = 1ULL;
            for (; n; --n) r *= 10;
            return r;
        }

        template<char...> struct hex_parse
            { static constexpr unsigned long long value = 0; };
        template<char F, char... R> struct hex_parse<F, R...> {
            static constexpr unsigned long long value =
                (hex_val(F) << (4 * sizeof...(R))) | hex_parse<R...>::value;
        };

        template<char...> struct dec_parse
            { static constexpr unsigned long long value = 0; };
        template<char F, char... R> struct dec_parse<F, R...> {
            static constexpr unsigned long long value =
                static_cast<unsigned long long>(F - '0') * pow10(sizeof...(R))
                + dec_parse<R...>::value;
        };

        template<char...> struct uint_value;
        template<char... R> struct uint_value<'0', 'x', R...> : hex_parse<R...> {};
        template<char... R> struct uint_value<'0', 'X', R...> : hex_parse<R...> {};
        template<char F, char... R> struct uint_value<F, R...> : dec_parse<F, R...> {};
        template<> struct uint_value<> { static constexpr unsigned long long value = 0; };

        template<unsigned long long V>
        constexpr auto deduce_le() noexcept {
            if constexpr (V <= 0xFFull) return le<u8>{static_cast<u8>(V)};
            else if constexpr (V <= 0xFFFFull) return le<u16>{static_cast<u16>(V)};
            else if constexpr (V <= 0xFFFFFFFFull) return le<u32>{static_cast<u32>(V)};
            else return le<u64>{static_cast<u64>(V)};
        }

        template<unsigned long long V>
        constexpr auto deduce_be() noexcept {
            if constexpr (V <= 0xFFull) return be<u8>{static_cast<u8>(V)};
            else if constexpr (V <= 0xFFFFull) return be<u16>{static_cast<u16>(V)};
            else if constexpr (V <= 0xFFFFFFFFull) return be<u32>{static_cast<u32>(V)};
            else return be<u64>{static_cast<u64>(V)};
        }
    }

    template<char... Cs>
    constexpr auto operator""_le() noexcept {
        return details::deduce_le<details::uint_value<Cs...>::value>();
    }

    template<char... Cs>
    constexpr auto operator""_be() noexcept {
        return details::deduce_be<details::uint_value<Cs...>::value>();
    }
}

