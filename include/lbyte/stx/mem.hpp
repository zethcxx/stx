#pragma once
#include "core.hpp"
#include "fn.hpp"
#include <bit>
#include <compare>
#include <cstring>
#include <functional>

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
    Type ccast( auto value ) noexcept {
        return const_cast<Type>( value );
    }

    template<class Type>
    STX_FORCE_INLINE
    Type dcast( auto value ) noexcept {
        return dynamic_cast<Type>( value );
    }

    // SAFE MEMORY ACCESS (memcpy, well-defined, unaligned-safe) -----------------
    template<binary_readable Type, address_like Addr, byte_offset OffT = off_s>
    [[nodiscard]] STX_FORCE_INLINE
    Type read( Addr base, OffT off = OffT{} ) noexcept
    {
        Type value;

        std::memcpy(
            &value,
            rcast<const std::byte*>(normalize_addr(base)) + off.get(),
            sizeof( Type )
        );

        return value;
    }

    // ENDIAN-AWARE READ -----------------------------------------------------
    template<byte_swappable Type, address_like Addr, byte_offset OffT = off_s>
    [[nodiscard]] STX_FORCE_INLINE
    Type read_le( Addr base, OffT off = OffT{} ) noexcept
    {
        return read<Type>( base, off );
    }

    template<byte_swappable Type, address_like Addr, byte_offset OffT = off_s>
    [[nodiscard]] STX_FORCE_INLINE
    Type read_be( Addr base, OffT off = OffT{} ) noexcept
    {
        using Raw = std::conditional_t<std::is_enum_v<Type>, std::underlying_type_t<Type>, Type>;
        if constexpr ( std::endian::native == std::endian::little )
            return static_cast<Type>( std::byteswap( static_cast<Raw>( read<Raw>( base, off ) ) ) );
        else
            return read<Type>( base, off );
    }

    template< binary_readable Type, address_like Addr, byte_offset OffT = off_s >
    STX_FORCE_INLINE
    void write( Addr base, OffT off, Type value ) noexcept
    {
        std::memcpy(
            rcast<std::byte*>(normalize_addr(base)) + off.get(),
            &value,
            sizeof(Type)
        );
    }

    // ENDIAN-AWARE WRITE ----------------------------------------------------
    template<byte_swappable Type, address_like Addr, byte_offset OffT = off_s>
    STX_FORCE_INLINE
    void write_le( Addr base, OffT off, Type value ) noexcept
    {
        write( base, off, value );
    }

    template<byte_swappable Type, address_like Addr>
    STX_FORCE_INLINE
    void write_le( Addr base, Type value ) noexcept
    {
        write_le( base, off_s{}, value );
    }

    template<byte_swappable Type, address_like Addr, byte_offset OffT = off_s>
    STX_FORCE_INLINE
    void write_be( Addr base, OffT off, Type value ) noexcept
    {
        using Raw = std::conditional_t<std::is_enum_v<Type>, std::underlying_type_t<Type>, Type>;
        if constexpr ( std::endian::native == std::endian::little )
            write( base, off, static_cast<Type>( std::byteswap( static_cast<Raw>( value ) ) ) );
        else
            write( base, off, value );
    }

    template<byte_swappable Type, address_like Addr>
    STX_FORCE_INLINE
    void write_be( Addr base, Type value ) noexcept
    {
        write_be( base, off_s{}, value );
    }

    // UNSAFE MEMORY ACCESS (direct deref, requires alignment, strict-aliasing) --
    template<binary_readable Type, byte_offset OffT = off_s>
    [[nodiscard]] STX_FORCE_INLINE
    Type read_raw( address_like auto base, OffT off = OffT{} ) noexcept
    {
        return *rcast<Type*>(
            rcast<std::byte*>(normalize_addr(base)) + off.get()
        );
    }

    template<binary_readable Type, byte_offset OffT = off_s>
    STX_FORCE_INLINE
    void write_raw( address_like auto base, OffT off, Type value ) noexcept
    {
        *rcast<Type*>(
            rcast<std::byte*>(normalize_addr(base)) + off.get()
        ) = value;
    }

    // --- ptr<T, Stride> ---------------------------------------------------------

    template<typename T, uptr Stride = 1>
    class ptr
    {
        ::lbyte::stx::uptr address = 0;

    public:
        using value_type = T;

        constexpr ptr() noexcept = default;

        constexpr explicit ptr(address_like auto addr) noexcept
          : address { normalize_addr( addr ) }
        {}

        constexpr ptr(null_t) noexcept
          : address { 0 }
        {}

        // ---- REBIND ADDRESS --------------------------------------

        constexpr ptr& operator=(address_like auto addr) noexcept {
            address = normalize_addr( addr );
            return *this;
        }

        // ---- CROSS-STRIDE ASSIGNMENT -----------------------------

        template<uptr OtherStride>
        constexpr ptr& operator=( const ptr<T, OtherStride>& other ) noexcept {
            address = other.addr();
            return *this;
        }

        // ---- BASE ------------------------------------------------

        [[nodiscard]]
        auto raw() noexcept -> T* {
            return rcast<T*>(address);
        }

        [[nodiscard]]
        auto raw() const noexcept -> const T* {
            return rcast<const T*>(address);
        }

        [[nodiscard]]
        constexpr ::lbyte::stx::uptr addr() const noexcept {
            return address;
        }

        [[nodiscard]]
        constexpr explicit operator bool() const noexcept {
            return address != 0;
        }

        [[nodiscard]]
        constexpr explicit operator ::lbyte::stx::uptr() const noexcept {
            return address;
        }

        [[nodiscard]]
        constexpr auto operator<=>( const ptr& ) const noexcept = default;

        [[nodiscard]]
        constexpr bool operator==( const ptr& ) const noexcept = default;

        [[nodiscard]] constexpr bool operator==(null_t) const noexcept { return address == 0; }
        [[nodiscard]] constexpr bool operator!=(null_t) const noexcept { return address != 0; }

        // ---- DEREFERENCE ------------------------------------------

        [[nodiscard]]
        auto operator*() noexcept -> std::add_lvalue_reference_t<T>
            requires ( not std::is_void_v<T> ) {
            return *rcast<T*>(address);
        }

        [[nodiscard]]
        auto operator*() const noexcept -> std::add_lvalue_reference_t<const T>
            requires ( not std::is_void_v<T> ) {
            return *rcast<const T*>(address);
        }

        // ---- ARROW ACCESS ----------------------------------------

        [[nodiscard]]
        auto operator->() noexcept -> T* {
            return rcast<T*>(address);
        }

        [[nodiscard]]
        auto operator->() const noexcept -> const T* {
            return rcast<const T*>(address);
        }

        // ---- NAVIGATION -------------------------------------------

        template<std::integral U>
        [[nodiscard]] constexpr ptr operator[]( U offset ) const noexcept {
            return ptr( address + static_cast<::lbyte::stx::uptr>( offset ));
        }

        template<byte_offset OffT>
        [[nodiscard]] constexpr ptr operator[]( OffT offset ) const noexcept {
            return ptr( address + static_cast<::lbyte::stx::uptr>( offset.get() ));
        }

        // ---- SAFE (memcpy) ---------------------------------------

        template<typename U = T>
        [[nodiscard]] STX_FORCE_INLINE
        auto read() const noexcept -> U
            requires ( not std::is_void_v<U> && binary_readable<U> )
        {
            U value;
            std::memcpy(
                &value,
                rcast<const std::byte*>(address),
                sizeof(U)
            );
            return value;
        }

        template<typename U = T>
        [[nodiscard]] STX_FORCE_INLINE
        auto read_p() const noexcept -> ptr<U>
            requires ( not std::is_void_v<U> )
        {
            ::lbyte::stx::uptr value;
            std::memcpy(
                &value,
                rcast<const std::byte*>(address),
                sizeof(::lbyte::stx::uptr)
            );
            return ptr<U>( rcast<U*>( value ));
        }

        template<typename U = T>
        STX_FORCE_INLINE
        void write( U value ) const noexcept
            requires ( not std::is_void_v<U> && binary_readable<U> )
        {
            std::memcpy(
                rcast<std::byte*>(address),
                &value,
                sizeof(U)
            );
        }

        template<std::integral U = T>
        STX_FORCE_INLINE
        void write_le( U value ) const noexcept
            requires ( not std::is_void_v<U> && binary_readable<U> )
        {
            ::lbyte::stx::write_le<U>( address, value );
        }

        template<std::integral U = T>
        STX_FORCE_INLINE
        void write_be( U value ) const noexcept
            requires ( not std::is_void_v<U> && binary_readable<U> )
        {
            ::lbyte::stx::write_be<U>( address, value );
        }

        // ---- UNSAFE (direct deref) --------------------------------

        template<typename U = T>
        [[nodiscard]] STX_FORCE_INLINE
        auto read_raw() const noexcept -> U
            requires ( not std::is_void_v<U> && binary_readable<U> )
        {
            return *rcast<U*>( address );
        }

        template<typename U = T>
        STX_FORCE_INLINE
        void write_raw( U value ) const noexcept
            requires ( not std::is_void_v<U> && binary_readable<U> )
        {
            *rcast<U*>( address ) = value;
        }

        // ---- TYPE REBIND -----------------------------------------

        template<typename U>
        [[nodiscard]] constexpr ptr<U> as_p() const noexcept {
            return ptr<U>(address);
        }

        template<typename U>
        [[nodiscard]] constexpr auto as() const noexcept -> U {
            return scast<U>(address);
        }

        // ---- ALIGNMENT -------------------------------------------

        template<std::unsigned_integral U = usize>
        [[nodiscard]] constexpr ptr align_up( U alignment ) const noexcept {
            return ptr( ::lbyte::stx::align_up( address, static_cast<usize>(alignment) ));
        }

        template<std::unsigned_integral U = usize>
        [[nodiscard]] constexpr ptr align_down( U alignment ) const noexcept {
            return ptr( ::lbyte::stx::align_down( address, static_cast<usize>(alignment) ));
        }

        // ---- INCREMENT / DECREMENT --------------------------------

        constexpr ptr& operator++() noexcept {
            ++address;
            return *this;
        }
        constexpr ptr operator++(int) noexcept {
            auto tmp = *this;
            ++address;
            return tmp;
        }
        constexpr ptr& operator--() noexcept {
            --address;
            return *this;
        }
        constexpr ptr operator--(int) noexcept {
            auto tmp = *this;
            --address;
            return tmp;
        }

        // ---- ARITHMETIC -------------------------------------------

        template<byte_offset OffT>
        [[nodiscard]] constexpr ptr operator+( OffT offset ) const noexcept {
            return ptr( address + static_cast<uptr>( offset.get() ));
        }

        template<byte_offset OffT>
        [[nodiscard]] constexpr ptr operator-( OffT offset ) const noexcept {
            return ptr( address - static_cast<uptr>( offset.get() ));
        }

        [[nodiscard]] constexpr off_s operator-( ptr other ) const noexcept {
            return off_s{ scast<off_s::value_type>( address - other.address ) };
        }

        template<byte_offset OffT>
        constexpr ptr& operator+=( OffT offset ) noexcept {
            address += static_cast<uptr>( offset.get() );
            return *this;
        }

        template<byte_offset OffT>
        constexpr ptr& operator-=( OffT offset ) noexcept {
            address -= static_cast<uptr>( offset.get() );
            return *this;
        }

        // ---- DISTANCE --------------------------------------------

        [[nodiscard]] constexpr off_s diff( ptr other ) const noexcept {
            return off_s{ scast<off_s::value_type>( address - other.address ) };
        }

        // ---- WALK (memcpy-safe pointer chasing) ------------------

        template<std::integral U>
        [[nodiscard]] STX_FORCE_INLINE
        ptr walk( U offset ) const noexcept {
            ::lbyte::stx::uptr target = address + static_cast<::lbyte::stx::uptr>( offset );
            ::lbyte::stx::uptr value;
            std::memcpy( &value, reinterpret_cast<const std::byte*>( target ), sizeof( value ));
            return ptr( value );
        }

        template<byte_offset OffT>
        [[nodiscard]] STX_FORCE_INLINE
        ptr walk( OffT offset ) const noexcept {
            return walk( offset.get() );
        }

        // ---- CHAIN: base / off / off -----------------------------

        template<std::integral U>
        [[nodiscard]] STX_FORCE_INLINE
        ptr operator>>( U offset ) const noexcept {
            ::lbyte::stx::uptr target = address + static_cast<::lbyte::stx::uptr>( offset ) * Stride;
            ::lbyte::stx::uptr value;
            std::memcpy( &value, reinterpret_cast<const std::byte*>( target ), sizeof( value ));
            return ptr( value );
        }

        template<byte_offset OffT>
        [[nodiscard]] STX_FORCE_INLINE
        ptr operator>>( OffT offset ) const noexcept {
            return operator>>( offset.get() );
        }

        // ---- STRIDE MANAGEMENT -----------------------------------

        template<::lbyte::stx::uptr NewStride>
        [[nodiscard]] constexpr ptr<T, NewStride> at() const noexcept {
            return ptr<T, NewStride>( address );
        }

        template<typename U>
        [[nodiscard]] constexpr ptr<T, sizeof(U)> at() const noexcept {
            return ptr<T, sizeof(U)>( address );
        }

        // ---- ALIGNMENT CHECK -------------------------------------

        template<typename U>
        [[nodiscard]] constexpr bool is_aligned() const noexcept {
            return ( address & ( static_cast<uptr>( alignof(U) ) - 1 )) == 0;
        }

        template<usize Alignment>
        [[nodiscard]] constexpr bool is_aligned() const noexcept {
            return ( address & ( Alignment - 1 )) == 0;
        }

        // ---- CALL ------------------------------------------------

        template<class Sig, class... Args>
        [[nodiscard]] inline constexpr decltype(auto) call(Args&&... args) const
            noexcept(std::is_nothrow_invocable_v<typename stx::caller_t<Sig>::fn_t, Args...>)
        {
            return stx::caller<Sig>(address)(std::forward<Args>(args)...);
        }

        template<class Sig>
        [[nodiscard]] inline constexpr auto caller() const noexcept
        {
            return stx::caller<Sig>(address);
        }

        // ---- SWAP ------------------------------------------------

        constexpr void swap( ptr& other ) noexcept {
            auto tmp = address;
            address = other.address;
            other.address = tmp;
        }

        friend constexpr void swap( ptr& a, ptr& b ) noexcept {
            a.swap( b );
        }
    };
}

#ifndef LBYTE_STX_MODULE
template<typename T, lbyte::stx::uptr Stride>
struct std::hash<lbyte::stx::ptr<T, Stride>>
{
    [[nodiscard]] auto operator()( const lbyte::stx::ptr<T, Stride>& p ) const noexcept {
        return std::hash<lbyte::stx::uptr>{}( p.addr() );
    }
};
#endif

#undef STX_FORCE_INLINE

