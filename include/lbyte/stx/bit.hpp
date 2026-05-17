#pragma once
#include "core.hpp"

#include <concepts>
#include <limits>
#include <type_traits>

namespace lbyte::stx
{
    // --- BIT OPERATIONS ----------------------------------------------------------

    template<usize Pos, usize Len = 1, std::unsigned_integral T>
    [[nodiscard]] constexpr T bit_extract(T value) noexcept
    {
        constexpr auto bits = std::numeric_limits<T>::digits;
        static_assert(Pos < bits, "bit position out of range");

        if constexpr (Len == 1)
            return static_cast<T>((value >> Pos) & 1);
        else if constexpr (Pos + Len >= bits)
            return value >> Pos;
        else
            return (value >> Pos) & static_cast<T>((T{1} << Len) - 1);
    }

    template<usize Pos, usize Len = 1, std::unsigned_integral T>
    [[nodiscard]] constexpr T bit_mask(T value) noexcept
    {
        constexpr auto bits = std::numeric_limits<T>::digits;
        static_assert(Pos < bits, "bit position out of range");

        if constexpr (Pos + Len >= bits)
            return value & static_cast<T>(~T{0} << Pos);
        else
            return value & static_cast<T>(((T{1} << Len) - 1) << Pos);
    }

    template<usize Pos, usize Len = 1, std::unsigned_integral T>
    constexpr void bit_insert(T& dest, std::unsigned_integral auto src) noexcept
    {
        constexpr auto bits = std::numeric_limits<T>::digits;
        static_assert(Pos < bits, "bit position out of range");

        auto s = static_cast<T>(src);

        if constexpr (Len == 1) {
            dest = (dest & ~(T{1} << Pos)) | ((s & 1) << Pos);
        } else if constexpr (Pos + Len >= bits) {
            constexpr auto clear_mask = static_cast<T>(~T{0} << Pos);
            dest = (dest & ~clear_mask) | (s << Pos);
        } else {
            constexpr auto mask = static_cast<T>(((T{1} << Len) - 1) << Pos);
            dest = (dest & ~mask) | ((s << Pos) & mask);
        }
    }

    template<usize Pos, std::unsigned_integral T>
    [[nodiscard]] constexpr bool bit_test(T value) noexcept
    {
        constexpr auto bits = std::numeric_limits<T>::digits;
        static_assert(Pos < bits, "bit position out of range");

        return (value >> Pos) & 1;
    }

    template<usize Pos, std::unsigned_integral T>
    [[nodiscard]] constexpr T bit_set(T value) noexcept
    {
        constexpr auto bits = std::numeric_limits<T>::digits;
        static_assert(Pos < bits, "bit position out of range");

        return value | (T{1} << Pos);
    }

    template<usize Pos, std::unsigned_integral T>
    [[nodiscard]] constexpr T bit_clear(T value) noexcept
    {
        constexpr auto bits = std::numeric_limits<T>::digits;
        static_assert(Pos < bits, "bit position out of range");

        return value & ~(T{1} << Pos);
    }

    template<usize Pos, std::unsigned_integral T>
    [[nodiscard]] constexpr T bit_flip(T value) noexcept
    {
        constexpr auto bits = std::numeric_limits<T>::digits;
        static_assert(Pos < bits, "bit position out of range");

        return value ^ (T{1} << Pos);
    }

    // --- BYTE OPERATIONS ---------------------------------------------------------

    template<usize N, std::unsigned_integral T>
    [[nodiscard]] constexpr u8 byte_extract(T value) noexcept
    {
        constexpr auto bytes = sizeof(T);
        static_assert(N < bytes, "byte index out of range");

        return static_cast<u8>(value >> (N * 8));
    }

    template<usize N, std::unsigned_integral T>
    constexpr void byte_insert(T& dest, std::unsigned_integral auto src) noexcept
    {
        constexpr auto bytes = sizeof(T);
        static_assert(N < bytes, "byte index out of range");

        constexpr T shift = static_cast<T>(N * 8);
        constexpr T mask  = static_cast<T>(T{0xFF} << shift);

        dest = (dest & ~mask) | (static_cast<T>(src) << shift);
    }

    template<usize N, std::unsigned_integral T>
    [[nodiscard]] constexpr T byte_mask(T value) noexcept
    {
        constexpr auto bytes = sizeof(T);
        static_assert(N < bytes, "byte index out of range");

        return value & static_cast<T>(T{0xFF} << (N * 8));
    }

    template<usize A, usize B, std::unsigned_integral T>
    constexpr void byte_swap(T& value) noexcept
    {
        constexpr auto bytes = sizeof(T);
        static_assert(A < bytes, "byte index A out of range");
        static_assert(B < bytes, "byte index B out of range");

        if constexpr (A != B) {
            auto a = static_cast<u8>(value >> (A * 8));
            auto b = static_cast<u8>(value >> (B * 8));

            constexpr u8 H = 0xFF;
            constexpr T mask_a = static_cast<T>(T{H} << (A * 8));
            constexpr T mask_b = static_cast<T>(T{H} << (B * 8));

            value = (value & ~mask_a & ~mask_b)
                  | (static_cast<T>(b) << (A * 8))
                  | (static_cast<T>(a) << (B * 8));
        }
    }
}
