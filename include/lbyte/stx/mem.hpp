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

    template<typename T = uptr, uptr Stride = 1> class wptr;

    template<typename T>
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
        constexpr auto raw() noexcept -> T* {
            return rcast<T*>(address);
        }

        [[nodiscard]]
        constexpr auto raw() const noexcept -> const T* {
            return rcast<const T*>(address);
        }

        [[nodiscard]]
        constexpr ::lbyte::stx::uptr uptr() const noexcept {
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

        // ---- ACCESS ----------------------------------------------

        [[nodiscard]]
        constexpr auto operator->() noexcept -> T* {
            return rcast<T*>(address);
        }

        [[nodiscard]]
        constexpr auto operator->() const noexcept -> const T* {
            return rcast<const T*>(address);
        }

        // ---- BINARY READ / WRITE ---------------------------------

        template<typename U = T>
        [[nodiscard]] STX_FORCE_INLINE
        auto read( off_s off = off_s{} ) const noexcept
            requires ( not std::is_void_v<U> && binary_readable<U> )
        {
            if constexpr (std::is_same_v<U, T>) {
                return ptr<T>(scast<::lbyte::stx::uptr>(*rcast<U*>(address + off.get())));
            } else {
                return *rcast<U*>(address + off.get());
            }
        }

        template<typename U = T>
        STX_FORCE_INLINE
        void write( off_s off, U value ) const noexcept
            requires ( not std::is_void_v<U> && binary_readable<U> )
        {
            *rcast<U*>( address + off.get() ) = value;
        }

        template<typename U = T>
        STX_FORCE_INLINE
        void write( U value ) const noexcept
            requires ( not std::is_void_v<U> && binary_readable<U> )
        {
            *rcast<U*>( address ) = value;
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

    template<typename T, uptr Stride>
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
        constexpr auto raw() noexcept -> T* {
            return reinterpret_cast<T*>(address);
        }

        [[nodiscard]]
        constexpr auto raw() const noexcept -> const T* {
            return reinterpret_cast<const T*>(address);
        }

        [[nodiscard]]
        constexpr ::lbyte::stx::uptr uptr() const noexcept {
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

        // ---- ARROW ACCESS -----------------------------------------

        [[nodiscard]]
        constexpr auto operator->() noexcept -> T* {
            return reinterpret_cast<T*>(address);
        }

        [[nodiscard]]
        constexpr auto operator->() const noexcept -> const T* {
            return reinterpret_cast<const T*>(address);
        }

        // ---- OFFSET ----------------------------------------------

        [[nodiscard]]
        constexpr wptr operator+(usize offset) const noexcept {
            return wptr(address + offset);
        }

        [[nodiscard]]
        constexpr wptr operator+(off_s offset) const noexcept {
            return wptr(address + scast<::lbyte::stx::uptr>(offset.get()));
        }

        // ---- WALK (memcpy-safe deref, byte offset) ---------------

        template<std::integral U>
        [[nodiscard]] STX_FORCE_INLINE
        wptr walk( U offset ) const noexcept {
            ::lbyte::stx::uptr target = address + static_cast<::lbyte::stx::uptr>(offset);
            ::lbyte::stx::uptr value;
            std::memcpy(&value, reinterpret_cast<const std::byte*>(target), sizeof(value));
            return wptr(value);
        }

        [[nodiscard]] STX_FORCE_INLINE
        wptr walk( off_s offset ) const noexcept {
            ::lbyte::stx::uptr target = address + static_cast<::lbyte::stx::uptr>(offset.get());
            ::lbyte::stx::uptr value;
            std::memcpy(&value, reinterpret_cast<const std::byte*>(target), sizeof(value));
            return wptr(value);
        }

        // sintaxis chain: base[off][off]
        template<std::integral U>
        [[nodiscard]] STX_FORCE_INLINE
        wptr operator[](U offset) const noexcept {
            ::lbyte::stx::uptr target = address + static_cast<::lbyte::stx::uptr>(offset) * Stride;
            ::lbyte::stx::uptr value;
            std::memcpy(&value, reinterpret_cast<const std::byte*>(target), sizeof(value));
            return wptr(value);
        }

        [[nodiscard]] STX_FORCE_INLINE
        wptr operator[](off_s offset) const noexcept {
            ::lbyte::stx::uptr target = address + static_cast<::lbyte::stx::uptr>(offset.get()) * Stride;
            ::lbyte::stx::uptr value;
            std::memcpy(&value, reinterpret_cast<const std::byte*>(target), sizeof(value));
            return wptr(value);
        }

        // ---- REBIND ----------------------------------------------

        template<typename U>
        [[nodiscard]] constexpr ptr<U> as() const noexcept {
            return ptr<U>(address);
        }

        template<typename U>
        [[nodiscard]] constexpr wptr<U, Stride> as_w() const noexcept {
            return wptr<U, Stride>(address);
        }

        template<::lbyte::stx::uptr NewStride>
        [[nodiscard]] constexpr wptr<T, NewStride> at() const noexcept {
            return wptr<T, NewStride>(address);
        }

        template<typename U>
        [[nodiscard]] constexpr wptr<T, sizeof(U)> at() const noexcept {
            return wptr<T, sizeof(U)>(address);
        }

        // ---- BINARY READ / WRITE ---------------------------------

        template<typename U = T>
        [[nodiscard]] STX_FORCE_INLINE
        auto read( off_s off = off_s{} ) const noexcept
            requires ( not std::is_void_v<U> && binary_readable<U> )
        {
            if constexpr (std::is_same_v<U, T>) {
                return ptr<T>(scast<::lbyte::stx::uptr>(*rcast<U*>(address + off.get())));
            } else {
                return *rcast<U*>(address + off.get());
            }
        }

        template<typename U = T>
        STX_FORCE_INLINE
        void write( off_s off, U value ) const noexcept
            requires ( not std::is_void_v<U> && binary_readable<U> )
        {
            *rcast<U*>( address + off.get() ) = value;
        }

        template<typename U = T>
        STX_FORCE_INLINE
        void write( U value ) const noexcept
            requires ( not std::is_void_v<U> && binary_readable<U> )
        {
            *rcast<U*>( address ) = value;
        }

        // ---- CALL ------------------------------------------------

        template<class Sig>
        [[nodiscard]] constexpr auto call() const noexcept {
            return caller<Sig>(address);
        }
    };
}

#undef STX_FORCE_INLINE
