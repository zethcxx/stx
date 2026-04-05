#pragma once
#include "core.hpp"
#include <bit>
#include <cstring>

#if defined(__GNUC__) || defined(__clang__)
    #define STX_FORCE_INLINE [[gnu::always_inline]] inline
#else
    #define STX_FORCE_INLINE inline
#endif

namespace lbyte::stx
{
    // CASTING -------------------------------------------------------------------
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

    template<class Type>
    STX_FORCE_INLINE
    constexpr Type ccast( auto value ) noexcept {
        return const_cast<Type>( value );
    }

    template<class Type>
    STX_FORCE_INLINE
    Type dcast( auto value ) noexcept {
        return dynamic_cast<Type>( value );
    }

    // MEMORY ACCESS -------------------------------------------------------------
    template<class Type, address_like Addr>
    [[nodiscard]] STX_FORCE_INLINE
    Type read( Addr base, off_s off = off_s{0} ) noexcept
    {
        static_assert(std::is_trivially_copyable_v<Type>);

        Type value;

        std::memcpy(
            &value,
            rcast<const std::byte*>(normalize_addr(base)) + off.get(),
            sizeof( Type )
        );

        return value;
    }

    template<class Type>
    [[nodiscard]] STX_FORCE_INLINE
    Type read_raw( address_like auto base, off_s off = off_s{0} ) noexcept
    {
        static_assert(std::is_trivially_copyable_v<Type>);

        return *rcast<Type*>(
            rcast<std::byte*>(normalize_addr(base)) + off.get()
        );
    }

    template< class Type, address_like Addr >
    STX_FORCE_INLINE
    void write( Addr base, off_s off, Type value ) noexcept
    {
        static_assert(std::is_trivially_copyable_v<Type>);

        std::memcpy(
            rcast<std::byte*>(normalize_addr(base)) + off.get(),
            &value,
            sizeof(Type)
        );
    }

    template<class Type>
    STX_FORCE_INLINE
    void write_raw( address_like auto base, off_s off, Type value ) noexcept
    {
        static_assert(std::is_trivially_copyable_v<Type>);

        *rcast<Type*>(
            rcast<std::byte*>(normalize_addr(base)) + off.get()
        ) = value;
    }
}

#undef STX_FORCE_INLINE
