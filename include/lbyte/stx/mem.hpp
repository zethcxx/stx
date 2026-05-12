#pragma once
#include "core.hpp"
#include "fn.hpp"
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


    // -- UNSAFE ---------------------------------------------------

    template<typename T = void> class wptr;

    template<typename T = void>
    class ptr
    {
        uptr address = 0;

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
        constexpr uptr raw() const noexcept {
            return address;
        }

        [[nodiscard]]
        constexpr explicit operator bool() const noexcept {
            return address != 0;
        }

        [[nodiscard]]
        constexpr operator uptr() const noexcept {
            return address;
        }

        // ---- ACCESS ----------------------------------------------

        template<typename U = T>
        [[nodiscard]] STX_FORCE_INLINE
        U* operator->() const noexcept
            requires ( not std::is_void_v<U> )
        {
            return reinterpret_cast<U*>(address);
        }

        // ---- BINARY READ / WRITE ---------------------------------

        template<typename U = T>
        [[nodiscard]] STX_FORCE_INLINE
        U read( off_s off = off_s{} ) const noexcept
            requires ( not std::is_void_v<U> && binary_readable<U> )
        {
            return *reinterpret_cast<U*>( address + off.get() );
        }

        template<typename U = T>
        STX_FORCE_INLINE
        void write( off_s off, U value ) const noexcept
            requires ( not std::is_void_v<U> && binary_readable<U> )
        {
            *reinterpret_cast<U*>( address + off.get() ) = value;
        }

        template<typename U = T>
        STX_FORCE_INLINE
        void write( U value ) const noexcept
            requires ( not std::is_void_v<U> && binary_readable<U> )
        {
            *reinterpret_cast<U*>( address ) = value;
        }

        // ---- TYPE REBIND -----------------------------------------

        template<typename U>
        [[nodiscard]] constexpr ptr<U> as() const noexcept {
            return ptr<U>(address);
        }

        template<typename U>
        [[nodiscard]] constexpr wptr<U> as_w() const noexcept {
            return wptr<U>(address);
        }

        // ---- CALL ------------------------------------------------

        template<class Sig>
        [[nodiscard]] constexpr auto call() const noexcept {
            return caller<Sig>(address);
        }
    };

    template<typename T = void>
    class wptr
    {
        uptr address = 0;

    public:
        using value_type = T;

        constexpr wptr() noexcept = default;

        constexpr explicit wptr(address_like auto addr) noexcept
          : address { normalize_addr( addr ) }
        {}

        // ---- BASE ------------------------------------------------

        [[nodiscard]]
        constexpr uptr raw() const noexcept {
            return address;
        }

        [[nodiscard]]
        constexpr explicit operator bool() const noexcept {
            return address != 0;
        }

        [[nodiscard]]
        constexpr operator uptr() const noexcept {
            return address;
        }

        // ---- OFFSET ----------------------------------------------

        [[nodiscard]]
        constexpr wptr operator+(usize offset) const noexcept {
            return wptr(address + offset);
        }

        [[nodiscard]]
        constexpr wptr operator+(off_s offset) const noexcept {
            return wptr(address + static_cast<uptr>(offset.get()));
        }

        // ---- WALK (deref as uptr*) -------------------------------

        template<std::unsigned_integral U>
        [[nodiscard]] STX_FORCE_INLINE
        wptr<> walk( U offset ) const noexcept {
            uptr target = address + static_cast<uptr>(offset);
            return wptr<>( *reinterpret_cast<uptr*>(target) );
        }

        [[nodiscard]] STX_FORCE_INLINE
        wptr<> walk( off_s offset ) const noexcept {
            uptr target = address + static_cast<uptr>(offset.get());
            return wptr<>( *reinterpret_cast<uptr*>(target) );
        }

        // sintaxis chain: base[off][off]
        template<std::unsigned_integral U>
        [[nodiscard]] STX_FORCE_INLINE
        wptr<> operator[](U offset) const noexcept {
            return walk(offset);
        }

        [[nodiscard]] STX_FORCE_INLINE
        wptr<> operator[](off_s offset) const noexcept {
            return walk(offset);
        }

        // ---- REBIND ----------------------------------------------

        template<typename U>
        [[nodiscard]] constexpr ptr<U> as() const noexcept {
            return ptr<U>(address);
        }

        template<typename U>
        [[nodiscard]] constexpr wptr<U> as_w() const noexcept {
            return wptr<U>(address);
        }

        // ---- DIRECT READ (optional) ------------------------------

        template<typename U = T>
        [[nodiscard]] STX_FORCE_INLINE
        U read() const noexcept
            requires ( not std::is_void_v<U> && binary_readable<U> )
        {
            return *reinterpret_cast<U*>(address);
        }

        // ---- CALL ------------------------------------------------

        template<class Sig>
        [[nodiscard]] constexpr auto call() const noexcept {
            return caller<Sig>(address);
        }
    };
}

#undef STX_FORCE_INLINE
