#pragma once
#include "./mem.hpp"
#include "./endian.hpp"

namespace lbyte::stx::literals
{
    // --- FLOATING TYPES ------------------------------------------------------
    constexpr float  operator""_f32( long double v ) noexcept { return static_cast<float>(v);  }
    constexpr float  operator""_f32( unsigned long long v ) noexcept { return static_cast<float>(v); }
    constexpr double operator""_f64( long double v ) noexcept { return static_cast<double>(v); }
    constexpr double operator""_f64( unsigned long long v ) noexcept { return static_cast<double>(v); }

    // --- INTEGER TYPES -------------------------------------------------------
    constexpr u8   operator""_u8  ( unsigned long long v ) noexcept { return static_cast<u8>(v);   }
    constexpr u16  operator""_u16 ( unsigned long long v ) noexcept { return static_cast<u16>(v);  }
    constexpr u32  operator""_u32 ( unsigned long long v ) noexcept { return static_cast<u32>(v);  }
    constexpr u64  operator""_u64 ( unsigned long long v ) noexcept { return static_cast<u64>(v);  }
    constexpr i8   operator""_i8  ( unsigned long long v ) noexcept { return static_cast<i8>(v);   }
    constexpr i16  operator""_i16 ( unsigned long long v ) noexcept { return static_cast<i16>(v);  }
    constexpr i32  operator""_i32 ( unsigned long long v ) noexcept { return static_cast<i32>(v);  }
    constexpr i64  operator""_i64 ( unsigned long long v ) noexcept { return static_cast<i64>(v);  }
    constexpr usize operator""_uz ( unsigned long long v ) noexcept { return static_cast<usize>(v); }

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
        return static_cast<usize>( v ) * 1024;
    }

    constexpr usize operator""_mb( unsigned long long v ) noexcept {
        return static_cast<usize>( v ) * 1024 * 1024;
    }

    constexpr usize operator""_gb( unsigned long long v ) noexcept {
        return static_cast<usize>( v ) * 1024 * 1024 * 1024;
    }

    // --- POINTERS (default to std::byte) ------------------------------------
    constexpr ptr<std::byte> operator""_ptr( unsigned long long v ) noexcept {
        return ptr<std::byte>{ static_cast<uptr>(v) };
    }

    constexpr wptr<std::byte, 1> operator""_wptr( unsigned long long v ) noexcept {
        return wptr<std::byte, 1>{ static_cast<uptr>(v) };
    }

    // --- ENDIAN TYPES -------------------------------------------------------
    constexpr le<u64> operator""_le( unsigned long long v ) noexcept {
        return le<u64>{ static_cast<u64>(v) };
    }

    constexpr be<u64> operator""_be( unsigned long long v ) noexcept {
        return be<u64>{ static_cast<u64>(v) };
    }
}
