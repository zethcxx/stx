#pragma once
#include "mem.hpp"
#include "endian.hpp"
#include "ct.hpp"

namespace lbyte::stx::literals
{
    using namespace endian;
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
    constexpr isize operator""_iz  ( unsigned long long v ) noexcept { return static_cast<isize>(v); }

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

    // --- SIZE LITERALS (powers of 1024, IEC binary prefixes) ---------------
    constexpr usize operator""_kib( unsigned long long v ) noexcept {
        return static_cast<usize>( v * 1024ULL );
    }

    constexpr usize operator""_mib( unsigned long long v ) noexcept {
        return static_cast<usize>( v * 1024ULL * 1024ULL );
    }

    constexpr usize operator""_gib( unsigned long long v ) noexcept {
        return static_cast<usize>( v * 1024ULL * 1024ULL * 1024ULL );
    }

    constexpr usize operator""_tib( unsigned long long v ) noexcept {
        return static_cast<usize>( v * 1024ULL * 1024ULL * 1024ULL * 1024ULL );
    }

    constexpr usize operator""_pib( unsigned long long v ) noexcept {
        return static_cast<usize>( v * 1024ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL );
    }

    // --- SIZE LITERALS (powers of 1000, SI decimal prefixes) ---------------
    constexpr usize operator""_kb( unsigned long long v ) noexcept {
        return static_cast<usize>( v * 1000ULL );
    }

    constexpr usize operator""_mb( unsigned long long v ) noexcept {
        return static_cast<usize>( v * 1000ULL * 1000ULL );
    }

    constexpr usize operator""_gb( unsigned long long v ) noexcept {
        return static_cast<usize>( v * 1000ULL * 1000ULL * 1000ULL );
    }

    constexpr usize operator""_tb( unsigned long long v ) noexcept {
        return static_cast<usize>( v * 1000ULL * 1000ULL * 1000ULL * 1000ULL );
    }

    constexpr usize operator""_pb( unsigned long long v ) noexcept {
        return static_cast<usize>( v * 1000ULL * 1000ULL * 1000ULL * 1000ULL * 1000ULL );
    }

    // --- POINTERS (default to std::byte) ------------------------------------
    constexpr ptr<std::byte> operator""_ptr( unsigned long long v ) noexcept {
        return ptr<std::byte>{ static_cast<uptr>(v) };
    }

    constexpr ptr<u8>  operator""_ptr8 ( unsigned long long v ) noexcept {
        return ptr<u8>{ static_cast<uptr>(v) };
    }

    constexpr ptr<u16> operator""_ptr16( unsigned long long v ) noexcept {
        return ptr<u16>{ static_cast<uptr>(v) };
    }

    constexpr ptr<u32> operator""_ptr32( unsigned long long v ) noexcept {
        return ptr<u32>{ static_cast<uptr>(v) };
    }

    constexpr ptr<u64> operator""_ptr64( unsigned long long v ) noexcept {
        return ptr<u64>{ static_cast<uptr>(v) };
    }

    constexpr ptr<void> operator""_ptrv( unsigned long long v ) noexcept {
        return ptr<void>{ static_cast<uptr>(v) };
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

    // --- ENDIAN TYPES (fixed-width) -----------------------------------------
    constexpr le<u16> operator""_le16( unsigned long long v ) noexcept {
        return le<u16>{ static_cast<u16>(v) };
    }

    constexpr le<u32> operator""_le32( unsigned long long v ) noexcept {
        return le<u32>{ static_cast<u32>(v) };
    }

    constexpr le<u64> operator""_le64( unsigned long long v ) noexcept {
        return le<u64>{ static_cast<u64>(v) };
    }

    constexpr be<u16> operator""_be16( unsigned long long v ) noexcept {
        return be<u16>{ static_cast<u16>(v) };
    }

    constexpr be<u32> operator""_be32( unsigned long long v ) noexcept {
        return be<u32>{ static_cast<u32>(v) };
    }

    constexpr be<u64> operator""_be64( unsigned long long v ) noexcept {
        return be<u64>{ static_cast<u64>(v) };
    }

    // --- STRING -> INTEGER (ASCII pack) ------------------------------------
    template<ct::fixed_string S>
    constexpr auto operator""_istr() noexcept {
        return ct::istr<S>;
    }

    template<ct::fixed_string S>
    constexpr auto operator""_istr_be() noexcept {
        return ct::istr<S, ct::endian::big>;
    }

    // --- STRING -> BYTE_BLOCK ----------------------------------------------
    template<ct::fixed_string S>
    constexpr auto operator""_vstr() noexcept {
        return ct::vstr<S>;
    }

}

