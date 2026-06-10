#pragma once
#include "core.hpp"
#include "fn.hpp"
#include <bit>
#include <compare>
#include <cstring>
#include <functional>
#include <memory>
#include <iterator>
#include <span>
#include <string_view>

#if defined(__GNUC__) || defined(__clang__)
    #define STX_FORCE_INLINE [[gnu::always_inline]] inline
#else
    #define STX_FORCE_INLINE inline
#endif

namespace lbyte::stx
{
    namespace mem {

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
        Type read_le( Addr base, OffT off ) noexcept
        {
            return read<Type>( base, off );
        }

        template<byte_swappable Type, address_like Addr>
        [[nodiscard]] STX_FORCE_INLINE
        Type read_le( Addr base ) noexcept
        {
            return read_le<Type>( base, off_s{} );
        }

        template<byte_swappable Type, address_like Addr, byte_offset OffT = off_s>
        [[nodiscard]] STX_FORCE_INLINE
        Type read_be( Addr base, OffT off ) noexcept
        {
            using Raw = std::conditional_t<std::is_enum_v<Type>, std::underlying_type_t<Type>, Type>;
            if constexpr ( std::endian::native == std::endian::little )
                return static_cast<Type>( std::byteswap( static_cast<Raw>( read<Raw>( base, off ) ) ) );
            else
                return read<Type>( base, off );
        }

        template<byte_swappable Type, address_like Addr>
        [[nodiscard]] STX_FORCE_INLINE
        Type read_be( Addr base ) noexcept
        {
            return read_be<Type>( base, off_s{} );
        }

        template< binary_readable Type, address_like Addr, byte_offset OffT = off_s >
        STX_FORCE_INLINE
        void write( Addr base, OffT off, const Type& value ) noexcept
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

            if constexpr ( std::endian::native == std::endian::little ) {
                auto swapped = std::byteswap( static_cast<Raw>( value ) );
                write<Raw>( base, off, swapped );
            } else {
                write<Type>( base, off, value );
            }
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

    } // namespace mem


    template<typename T, uptr Stride = 1>
    struct ptr_light;

    template<typename T, uptr Stride = 1>
    class ptr
    {
        ::lbyte::stx::uptr address = 0;

    public:
        using value_type = T;

        constexpr ptr() noexcept = default;

        [[nodiscard]] constexpr ptr(T* raw_ptr) noexcept
          : address { rcast<::lbyte::stx::uptr>(raw_ptr) }
        {}

        constexpr explicit ptr(address_like auto addr) noexcept
          : address { normalize_addr( addr ) }
        {}

        constexpr ptr(null_t) noexcept
          : address { 0 }
        {}

        // ---- REBIND ADDRESS --------------------------------------

        ptr& operator=(T* raw_ptr) noexcept {
            address = rcast<::lbyte::stx::uptr>(raw_ptr);
            return *this;
        }

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
        auto operator->() noexcept -> T*
            requires ( not std::is_void_v<T> ) {
            return rcast<T*>(address);
        }

        [[nodiscard]]
        auto operator->() const noexcept -> const T*
            requires ( not std::is_void_v<T> ) {
            return rcast<const T*>(address);
        }

        // ---- NAVIGATION -------------------------------------------

        template<std::integral U>
        [[nodiscard]] constexpr ptr_light<T, Stride> operator[]( U offset ) const noexcept {
            return ptr_light<T, Stride>( address + static_cast<::lbyte::stx::uptr>( offset ));
        }

        template<byte_offset OffT>
        [[nodiscard]] constexpr ptr_light<T, Stride> operator[]( OffT offset ) const noexcept {
            return ptr_light<T, Stride>( address + static_cast<::lbyte::stx::uptr>( offset.get() ));
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

        template<bounded_array U>
        [[nodiscard]] STX_FORCE_INLINE
        auto read() const noexcept -> details::bounded_array_t<U>
        {
            details::bounded_array_t<U> arr;
            std::memcpy( &arr, rcast<const std::byte*>(address), sizeof(arr) );
            return arr;
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

        // ---- POP (read + advance) ---------------------------------

        template<typename U = T>
        [[nodiscard]] STX_FORCE_INLINE
        auto pop() noexcept -> U
            requires ( not std::is_void_v<U> && binary_readable<U> )
        {
            U value;
            std::memcpy( &value, rcast<const std::byte*>(address), sizeof(U) );
            address += sizeof(U);
            return value;
        }

        template<bounded_array U>
        [[nodiscard]] STX_FORCE_INLINE
        auto pop() noexcept -> details::bounded_array_t<U>
        {
            details::bounded_array_t<U> arr;
            std::memcpy( &arr, rcast<const std::byte*>(address), sizeof(arr) );
            address += sizeof(arr);
            return arr;
        }

        // ---- READ INTO (no advance) / POP INTO (advance) -----------

        template<writable_buffer R>
        STX_FORCE_INLINE
        void read_into( R&& buf ) const noexcept
        {
            auto const bytes = std::size(buf) * sizeof(*std::data(buf));
            std::memcpy(
                rcast<std::byte*>(std::data(buf)),
                rcast<const std::byte*>(address),
                static_cast<usize>(bytes)
            );
        }

        template<writable_buffer R>
        STX_FORCE_INLINE
        ptr& pop_into( R&& buf ) noexcept
        {
            auto const bytes = std::size(buf) * sizeof(*std::data(buf));
            std::memcpy(
                rcast<std::byte*>(std::data(buf)),
                rcast<const std::byte*>(address),
                static_cast<usize>(bytes)
            );
            address += static_cast<usize>(bytes);
            return *this;
        }

        // ---- WRITE (no advance) -----------------------------------

        template<typename U = T>
        STX_FORCE_INLINE
        void write( U value ) const noexcept
            requires ( not std::is_void_v<U> && binary_readable<U> && !contiguous_buffer<U> )
        {
            std::memcpy( rcast<std::byte*>(address), &value, sizeof(U) );
        }

        template<contiguous_buffer R>
        STX_FORCE_INLINE
        void write( R&& range ) const noexcept
        {
            auto const bytes = std::size(range) * sizeof(*std::data(range));
            std::memcpy(
                rcast<std::byte*>(address),
                rcast<const std::byte*>(std::data(range)),
                static_cast<usize>(bytes)
            );
        }

        template<byte_swappable U = T>
        STX_FORCE_INLINE
        void write_le( U value ) const noexcept
            requires ( not std::is_void_v<U> && binary_readable<U> )
        {
            ::lbyte::stx::mem::write_le<U>( address, value );
        }

        template<std::integral U = T>
        STX_FORCE_INLINE
        void write_be( U value ) const noexcept
            requires ( not std::is_void_v<U> && binary_readable<U> )
        {
            ::lbyte::stx::mem::write_be<U>( address, value );
        }

        // ---- PUSH (write + advance) -------------------------------

        template<typename U = T>
        STX_FORCE_INLINE
        ptr& push( const U& value ) noexcept
            requires ( not std::is_void_v<U> && binary_readable<U> && !contiguous_buffer<U> )
        {
            std::memcpy( rcast<std::byte*>(address), &value, sizeof(U) );
            address += sizeof(U);
            return *this;
        }

        template<contiguous_buffer R>
        STX_FORCE_INLINE
        ptr& push( R&& range ) noexcept
        {
            auto const bytes = std::size(range) * sizeof(*std::data(range));
            std::memcpy(
                rcast<std::byte*>(address),
                rcast<const std::byte*>(std::data(range)),
                static_cast<usize>(bytes)
            );
            address += static_cast<usize>(bytes);
            return *this;
        }

        // ---- READ (endian-aware, no advance) -----------------------

        template<std::integral U = T>
        [[nodiscard]] STX_FORCE_INLINE
        auto read_le() const noexcept -> U
            requires ( not std::is_void_v<U> && binary_readable<U> )
        {
            return ::lbyte::stx::mem::read_le<U>( address );
        }

        template<std::integral U = T>
        [[nodiscard]] STX_FORCE_INLINE
        auto read_be() const noexcept -> U
            requires ( not std::is_void_v<U> && binary_readable<U> )
        {
            return ::lbyte::stx::mem::read_be<U>( address );
        }

        // ---- ZERO-COPY VIEW (no advance) --------------------------

        template<bounded_array U>
        [[nodiscard]] STX_FORCE_INLINE
        auto as_view() const noexcept
            -> std::span<const std::remove_all_extents_t<U>>
        {
            using element_type = std::remove_all_extents_t<U>;
            using flat_array = details::bounded_array_t<U>;
            return std::span<const element_type>(
                rcast<const element_type*>(address),
                sizeof(flat_array) / sizeof(element_type)
            );
        }

        template<typename U = T>
            requires ( not std::is_void_v<U> && binary_readable<U> )
        [[nodiscard]] STX_FORCE_INLINE
        auto as_view( usize count ) const noexcept -> std::span<const U>
        {
            return std::span<const U>( rcast<const U*>( address ), count );
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
            if constexpr ( std::is_void_v<T> )
                address += Stride;
            else
                address += ( sizeof(T) * Stride );

            return *this;
        }

        constexpr ptr operator++(int) noexcept {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        constexpr ptr& operator--() noexcept {
            if constexpr ( std::is_void_v<T> )
                address -= Stride;
            else
                address -= ( sizeof(T) * Stride );

            return *this;
        }

        constexpr ptr operator--(int) noexcept {
            auto tmp = *this;
            --(*this);
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

        template<address_like Addr>
        [[nodiscard]] constexpr off_s diff( Addr other ) const noexcept {
            return off_s{ scast<off_s::value_type>( address - normalize_addr(other) ) };
        }

        // ---- WALK (memcpy-safe pointer chasing) ------------------

        template<typename ReturnType = T, byte_offset OffT>
        [[nodiscard]] STX_FORCE_INLINE
        auto walk( OffT offset ) const noexcept -> ptr<ReturnType, Stride>
            requires ( not std::is_void_v<ReturnType> )
        {
            ::lbyte::stx::uptr target = address + static_cast<::lbyte::stx::uptr>( offset.get() );
            ReturnType value;
            std::memcpy( &value, rcast<const std::byte*>( target ), sizeof( ReturnType ));
            return ptr<ReturnType, Stride>( rcast<ReturnType*>( value ) );
        }

        template<typename ReturnType = T, byte_offset OffT>
        [[nodiscard]] STX_FORCE_INLINE
        auto walk( OffT offset ) const noexcept -> ptr<void, Stride>
            requires ( std::is_void_v<ReturnType> )
        {
            ::lbyte::stx::uptr target = address + static_cast<::lbyte::stx::uptr>( offset.get() );
            ::lbyte::stx::uptr value;
            std::memcpy( &value, rcast<const std::byte*>( target ), sizeof( ::lbyte::stx::uptr ));
            return ptr<void, Stride>( value );
        }

        template<typename ReturnType = T, std::integral U>
        [[nodiscard]] STX_FORCE_INLINE
        auto walk( U offset ) const noexcept {
            return walk<ReturnType>( off_s{ offset } );
        }

        // ---- CHAIN: base / off / off -----------------------------

        template<byte_offset OffT>
        [[nodiscard]] STX_FORCE_INLINE
        auto operator>>( OffT offset ) const noexcept -> ptr<T, Stride>
            requires ( not std::is_void_v<T> )
        {
            ::lbyte::stx::uptr target = address + ( static_cast<::lbyte::stx::uptr>( offset.get() ) * Stride );
            T value;
            std::memcpy( &value, rcast<const std::byte*>( target ), sizeof( T ));
            return ptr<T, Stride>( rcast<T*>( value ) );
        }

        template<byte_offset OffT>
        [[nodiscard]] STX_FORCE_INLINE
        auto operator>>( OffT offset ) const noexcept -> ptr<void, Stride>
            requires ( std::is_void_v<T> )
        {
            ::lbyte::stx::uptr target = address + ( static_cast<::lbyte::stx::uptr>( offset.get() ) * Stride );
            ::lbyte::stx::uptr value;
            std::memcpy( &value, rcast<const std::byte*>( target ), sizeof( ::lbyte::stx::uptr ));
            return ptr<void, Stride>( value );
        }

        template<std::integral U>
        [[nodiscard]] STX_FORCE_INLINE
        auto operator>>( U offset ) const noexcept {
            return operator>>( off_s{ offset } );
        }

        // ---- STRIDE MANAGEMENT -----------------------------------

        template<::lbyte::stx::uptr NewStride>
        [[nodiscard]] constexpr ptr<T, NewStride> step() const noexcept {
            return ptr<T, NewStride>( address );
        }

        template<typename U>
        [[nodiscard]] constexpr ptr<T, sizeof(U)> step() const noexcept {
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

    // --- ptr_light<T, Stride> (non-chainable proxy from operator[]) -------------

    template<typename T, uptr Stride>
    struct ptr_light : ptr<T, Stride>
    {
        using ptr<T, Stride>::ptr;

        template<typename U>
        ptr_light operator[]( U ) const = delete;
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

template<typename T, lbyte::stx::uptr Stride>
struct std::hash<lbyte::stx::ptr_light<T, Stride>>
{
    [[nodiscard]] auto operator()( const lbyte::stx::ptr_light<T, Stride>& p ) const noexcept {
        return std::hash<lbyte::stx::uptr>{}( p.addr() );
    }
};
#endif

#undef STX_FORCE_INLINE

