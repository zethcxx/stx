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
    template<binary_readable Type, address_like Addr>
    [[nodiscard]] STX_FORCE_INLINE
    Type read( Addr base, off_s off = off_s{0} ) noexcept
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
    template<std::integral Type, address_like Addr>
    [[nodiscard]] STX_FORCE_INLINE
    Type read_le( Addr base, off_s off = off_s{0} ) noexcept
    {
        return read<Type>( base, off );
    }

    template<std::integral Type, address_like Addr>
    [[nodiscard]] STX_FORCE_INLINE
    Type read_be( Addr base, off_s off = off_s{0} ) noexcept
    {
        if constexpr ( std::endian::native == std::endian::little )
            return std::byteswap( read<Type>( base, off ) );
        else
            return read<Type>( base, off );
    }

    template< binary_readable Type, address_like Addr >
    STX_FORCE_INLINE
    void write( Addr base, off_s off, Type value ) noexcept
    {
        std::memcpy(
            rcast<std::byte*>(normalize_addr(base)) + off.get(),
            &value,
            sizeof(Type)
        );
    }

    // ENDIAN-AWARE WRITE ----------------------------------------------------
    template<std::integral Type, address_like Addr>
    STX_FORCE_INLINE
    void write_le( Addr base, off_s off, Type value ) noexcept
    {
        write( base, off, value );
    }

    template<std::integral Type, address_like Addr>
    STX_FORCE_INLINE
    void write_be( Addr base, off_s off, Type value ) noexcept
    {
        if constexpr ( std::endian::native == std::endian::little )
            write( base, off, std::byteswap( value ) );
        else
            write( base, off, value );
    }

    // UNSAFE MEMORY ACCESS (direct deref, requires alignment, strict-aliasing) --
    template<binary_readable Type>
    [[nodiscard]] STX_FORCE_INLINE
    Type read_raw( address_like auto base, off_s off = off_s{0} ) noexcept
    {
        return *rcast<Type*>(
            rcast<std::byte*>(normalize_addr(base)) + off.get()
        );
    }

    template<binary_readable Type>
    STX_FORCE_INLINE
    void write_raw( address_like auto base, off_s off, Type value ) noexcept
    {
        *rcast<Type*>(
            rcast<std::byte*>(normalize_addr(base)) + off.get()
        ) = value;
    }

    // --- FORWARD DECLARATIONS --------------------------------------------------

    template<typename T = uptr, uptr Stride = 1> class wptr;

    // --- ptr<T> ----------------------------------------------------------------

    template<typename T>
    class ptr
    {
        ::lbyte::stx::uptr address = 0;

    public:
        using value_type = T;

        constexpr ptr() noexcept = default;

        constexpr explicit ptr(address_like auto addr) noexcept
          : address { normalize_addr( addr ) }
        {}

        // ---- REBIND ADDRESS --------------------------------------

        constexpr ptr& operator=(address_like auto addr) noexcept {
            address = normalize_addr( addr );
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
        constexpr ::lbyte::stx::uptr& ref() noexcept {
            return address;
        }

        [[nodiscard]]
        constexpr explicit operator bool() const noexcept {
            return address != 0;
        }

        [[nodiscard]]
        constexpr operator ::lbyte::stx::uptr() const noexcept {
            return address;
        }

        [[nodiscard]]
        constexpr auto operator<=>( const ptr& ) const noexcept = default;

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

        template<typename U = T>
        [[nodiscard]] auto operator[]( off_s off ) noexcept -> U&
            requires ( not std::is_void_v<U> )
        {
            return *rcast<U*>( address + off.get() );
        }

        template<typename U = T>
        [[nodiscard]] auto operator[]( off_s off ) const noexcept -> const U&
            requires ( not std::is_void_v<U> )
        {
            return *rcast<const U*>( address + off.get() );
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

        // ---- SAFE (memcpy) ---------------------------------------

        [[nodiscard]] STX_FORCE_INLINE
        auto read( off_s off = off_s{} ) const noexcept -> ptr<T>
        {
            uptr value;
            std::memcpy(
                &value,
                rcast<const std::byte*>(address) + off.get(),
                sizeof(uptr)
            );
            return ptr<T>( rcast<T*>( value ));
        }

        template<binary_readable U>
        [[nodiscard]] STX_FORCE_INLINE
        auto read( off_s off = off_s{} ) const noexcept -> U
            requires ( not std::is_void_v<U> )
        {
            U value;
            std::memcpy(
                &value,
                rcast<const std::byte*>(address) + off.get(),
                sizeof(U)
            );
            return value;
        }

        template<std::integral U = T>
        [[nodiscard]] STX_FORCE_INLINE
        auto read_le( off_s off = off_s{} ) const noexcept -> U
            requires ( not std::is_void_v<U> && binary_readable<U> )
        {
            return ::lbyte::stx::read_le<U>( address, off );
        }

        template<std::integral U = T>
        [[nodiscard]] STX_FORCE_INLINE
        auto read_be( off_s off = off_s{} ) const noexcept -> U
            requires ( not std::is_void_v<U> && binary_readable<U> )
        {
            return ::lbyte::stx::read_be<U>( address, off );
        }

        template<typename U = T>
        STX_FORCE_INLINE
        void write( off_s off, U value ) const noexcept
            requires ( not std::is_void_v<U> && binary_readable<U> )
        {
            std::memcpy(
                rcast<std::byte*>(address) + off.get(),
                &value,
                sizeof(U)
            );
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
        void write_le( off_s off, U value ) const noexcept
            requires ( not std::is_void_v<U> && binary_readable<U> )
        {
            ::lbyte::stx::write_le<U>( address, off, value );
        }

        template<std::integral U = T>
        STX_FORCE_INLINE
        void write_be( off_s off, U value ) const noexcept
            requires ( not std::is_void_v<U> && binary_readable<U> )
        {
            ::lbyte::stx::write_be<U>( address, off, value );
        }

        // ---- UNSAFE (direct deref) --------------------------------

        template<typename U = T>
        [[nodiscard]] STX_FORCE_INLINE
        auto read_raw( off_s off = off_s{} ) const noexcept -> U
            requires ( not std::is_void_v<U> && binary_readable<U> )
        {
            return *rcast<U*>( address + off.get() );
        }

        template<typename U = T>
        STX_FORCE_INLINE
        void write_raw( off_s off, U value ) const noexcept
            requires ( not std::is_void_v<U> && binary_readable<U> )
        {
            *rcast<U*>( address + off.get() ) = value;
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
        [[nodiscard]] constexpr wptr<U> as_w() const noexcept {
            return wptr<U>(address);
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

        // ---- STRONG TYPE CONVERSIONS -----------------------------

        [[nodiscard]] constexpr off_s off() const noexcept {
            return off_s{ scast<off_s::value_type>( address ) };
        }

        [[nodiscard]] constexpr rva_s rva() const noexcept {
            return rva_s{ static_cast<rva_s::value_type>( address ) };
        }

        [[nodiscard]] constexpr va_s va() const noexcept {
            return va_s{ address };
        }

        // ---- ARITHMETIC -------------------------------------------

        [[nodiscard]] constexpr ptr add( off_s offset ) const noexcept {
            return ptr( address + static_cast<uptr>( offset.get() ));
        }

        [[nodiscard]] constexpr ptr sub( off_s offset ) const noexcept {
            return ptr( address - static_cast<uptr>( offset.get() ));
        }

        [[nodiscard]] constexpr ptr add( rva_s offset ) const noexcept {
            return ptr( address + static_cast<uptr>( offset.get() ));
        }

        [[nodiscard]] constexpr ptr operator+( off_s offset ) const noexcept {
            return ptr( address + static_cast<uptr>( offset.get() ));
        }

        [[nodiscard]] constexpr ptr operator+( rva_s offset ) const noexcept {
            return ptr( address + static_cast<uptr>( offset.get() ));
        }

        [[nodiscard]] constexpr ptr operator-( off_s offset ) const noexcept {
            return ptr( address - static_cast<uptr>( offset.get() ));
        }

        [[nodiscard]] off_s operator-( ptr other ) const noexcept {
            return off_s{ scast<off_s::value_type>( address - other.address ) };
        }

        constexpr ptr& operator+=( off_s offset ) noexcept {
            address += static_cast<uptr>( offset.get() );
            return *this;
        }

        constexpr ptr& operator-=( off_s offset ) noexcept {
            address -= static_cast<uptr>( offset.get() );
            return *this;
        }

        // ---- DISTANCE --------------------------------------------

        [[nodiscard]] off_s diff( ptr other ) const noexcept {
            return off_s{ scast<off_s::value_type>( address - other.address ) };
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

        template<class Sig>
        [[nodiscard]] auto call() const noexcept {
            return caller<Sig>(address);
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

    // --- wptr<T, Stride> -------------------------------------------------------

    template<typename T, uptr Stride>
    class wptr : public ptr<T>
    {
        using base = ptr<T>;

    public:
        using value_type = T;

        constexpr wptr() noexcept = default;

        constexpr explicit wptr(address_like auto addr) noexcept
            : base( addr )
        {}

        // ---- ARITHMETIC -------------------------------------------

        [[nodiscard]]
        constexpr wptr add( off_s offset ) const noexcept {
            return wptr( this->addr() + scast<::lbyte::stx::uptr>( offset.get() ));
        }

        [[nodiscard]]
        constexpr wptr add( rva_s offset ) const noexcept {
            return wptr( this->addr() + static_cast<::lbyte::stx::uptr>( offset.get() ));
        }

        [[nodiscard]]
        constexpr wptr sub( off_s offset ) const noexcept {
            return wptr( this->addr() - scast<::lbyte::stx::uptr>( offset.get() ));
        }

        [[nodiscard]]
        constexpr wptr operator+( off_s offset ) const noexcept {
            return wptr( this->addr() + scast<::lbyte::stx::uptr>( offset.get() ));
        }

        [[nodiscard]]
        constexpr wptr operator+( rva_s offset ) const noexcept {
            return wptr( this->addr() + static_cast<::lbyte::stx::uptr>( offset.get() ));
        }

        [[nodiscard]]
        constexpr wptr operator-( off_s offset ) const noexcept {
            return wptr( this->addr() - scast<::lbyte::stx::uptr>( offset.get() ));
        }

        constexpr wptr& operator+=( off_s offset ) noexcept {
            this->ref() += static_cast<::lbyte::stx::uptr>( offset.get() );
            return *this;
        }

        constexpr wptr& operator-=( off_s offset ) noexcept {
            this->ref() -= static_cast<::lbyte::stx::uptr>( offset.get() );
            return *this;
        }

        // ---- WALK (memcpy-safe pointer chasing) ------------------

        template<std::integral U>
        [[nodiscard]] STX_FORCE_INLINE
        wptr walk( U offset ) const noexcept {
            ::lbyte::stx::uptr target = this->addr() + static_cast<::lbyte::stx::uptr>( offset );
            ::lbyte::stx::uptr value;
            std::memcpy( &value, reinterpret_cast<const std::byte*>( target ), sizeof( value ));
            return wptr( value );
        }

        [[nodiscard]] STX_FORCE_INLINE
        wptr walk( off_s offset ) const noexcept {
            return walk( offset.get() );
        }

        // ---- CHAIN SYNTAX: base[off][off] ------------------------

        template<std::integral U>
        [[nodiscard]] STX_FORCE_INLINE
        wptr operator[]( U offset ) const noexcept {
            ::lbyte::stx::uptr target = this->addr() + static_cast<::lbyte::stx::uptr>( offset ) * Stride;
            ::lbyte::stx::uptr value;
            std::memcpy( &value, reinterpret_cast<const std::byte*>( target ), sizeof( value ));
            return wptr( value );
        }

        [[nodiscard]] STX_FORCE_INLINE
        wptr operator[]( off_s offset ) const noexcept {
            return (*this)[ offset.get() ];
        }

        // ---- REBIND ----------------------------------------------

        template<typename U>
        [[nodiscard]] constexpr wptr<U, Stride> as_w() const noexcept {
            return wptr<U, Stride>( this->addr() );
        }

        template<typename U>
        [[nodiscard]] constexpr auto as() const noexcept -> U {
            return scast<U>( this->addr() );
        }

        // ---- ALIGNMENT -------------------------------------------

        template<std::unsigned_integral U = usize>
        [[nodiscard]] constexpr wptr align_up( U alignment ) const noexcept {
            return wptr( ::lbyte::stx::align_up( this->addr(), static_cast<usize>(alignment) ));
        }

        template<std::unsigned_integral U = usize>
        [[nodiscard]] constexpr wptr align_down( U alignment ) const noexcept {
            return wptr( ::lbyte::stx::align_down( this->addr(), static_cast<usize>(alignment) ));
        }

        template<::lbyte::stx::uptr NewStride>
        [[nodiscard]] constexpr wptr<T, NewStride> at() const noexcept {
            return wptr<T, NewStride>( this->addr() );
        }

        template<typename U>
        [[nodiscard]] constexpr wptr<T, sizeof(U)> at() const noexcept {
            return wptr<T, sizeof(U)>( this->addr() );
        }
    };
}

#ifndef LBYTE_STX_MODULE
template<typename T>
struct std::hash<lbyte::stx::ptr<T>>
{
    [[nodiscard]] auto operator()( const lbyte::stx::ptr<T>& p ) const noexcept {
        return std::hash<lbyte::stx::uptr>{}( p.addr() );
    }
};

template<typename T, lbyte::stx::uptr Stride>
struct std::hash<lbyte::stx::wptr<T, Stride>>
{
    [[nodiscard]] auto operator()( const lbyte::stx::wptr<T, Stride>& p ) const noexcept {
        return std::hash<lbyte::stx::uptr>{}( p.addr() );
    }
};
#endif

#undef STX_FORCE_INLINE
