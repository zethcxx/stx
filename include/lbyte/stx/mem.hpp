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
#include <memory>

#if defined(__GNUC__) || defined(__clang__)
    #define STX_FORCE_INLINE [[gnu::always_inline]] inline
#else
    #define STX_FORCE_INLINE inline
#endif

namespace lbyte::stx
{
    namespace details {
        template<typename T>
        struct raw_for_endian { using type = T; };

        template<typename T> requires std::is_enum_v<T>
        struct raw_for_endian<T> { using type = std::underlying_type_t<T>; };
    }

    namespace mem {

        // SAFE MEMORY ACCESS (memcpy, well-defined, unaligned-safe) -----------------
        template<binary_readable Type, address_like Addr>
        [[nodiscard]] STX_FORCE_INLINE
        constexpr Type read( Addr base ) noexcept
        {
            Type value;
            std::memcpy(
                &value,
                rcast<const std::byte*>(normalize_addr(base)),
                sizeof(Type)
            );
            return value;
        }

        // ENDIAN-AWARE READ -----------------------------------------------------
        template<byte_swappable Type, address_like Addr>
        [[nodiscard]] STX_FORCE_INLINE
        Type read_le( Addr base ) noexcept
        {
            return read<Type>( base );
        }

        template<byte_swappable Type, address_like Addr>
        [[nodiscard]] STX_FORCE_INLINE constexpr Type read_be( Addr base ) noexcept
        {
            using Raw = details::raw_for_endian<Type>::type;
            auto raw = read<Raw>(base);
            if constexpr ( std::endian::native == std::endian::little )
                raw = std::byteswap(raw);
            return static_cast<Type>(raw);
        }

        template<binary_readable Type, address_like Addr>
            requires (not contiguous_buffer<Type>)
        STX_FORCE_INLINE
        void write( Addr base, const Type& value ) noexcept
        {
            std::memcpy(
                rcast<std::byte*>(normalize_addr(base)),
                &value,
                sizeof(Type)
            );
        }

        template<binary_readable Type, address_like Addr>
            requires (not contiguous_buffer<Type>)
        STX_FORCE_INLINE
        void write( Addr base, const off_s offset, const Type& value ) noexcept
        {
            write(
                rcast<std::byte*>(normalize_addr(base)) + offset.get(),
                value
            );
        }

        template<address_like Addr, contiguous_buffer R>
        STX_FORCE_INLINE
        void write( Addr base, R&& range ) noexcept
        {
            auto const bytes = std::size(range) * sizeof(*std::data(range));
            std::memcpy(
                rcast<std::byte*>(normalize_addr(base)),
                rcast<const std::byte*>(std::data(range)),
                static_cast<usize>(bytes)
            );
        }

        template<address_like Addr, contiguous_buffer R>
        STX_FORCE_INLINE
        void write( Addr base, const off_s offset, const R& range ) noexcept
        {
            auto const bytes = std::size(range) * sizeof(*std::data(range));
            std::memcpy(
                rcast<std::byte*>(normalize_addr(base)) + offset.get(),
                rcast<const std::byte*>(std::data(range)),
                static_cast<usize>(bytes)
            );
        }

        // WRITE INTO WRITABLE BUFFER (span, vector, array...)
        template<writable_buffer Dest, binary_readable Type>
            requires (not contiguous_buffer<Type>)
        STX_FORCE_INLINE
        void write( Dest&& dest, const off_s offset, const Type& value ) noexcept
        {
            std::memcpy(
                rcast<std::byte*>(std::data(dest)) + offset.get(),
                &value,
                sizeof(Type)
            );
        }

        template<writable_buffer Dest, binary_readable Type>
            requires (not contiguous_buffer<Type>)
        STX_FORCE_INLINE
        void write( Dest&& dest, const Type& value ) noexcept
        {
            write(std::forward<Dest>(dest), off_s{0}, value);
        }

        template<writable_buffer Dest, contiguous_buffer R>
        STX_FORCE_INLINE
        void write( Dest&& dest, const off_s offset, const R& range ) noexcept
        {
            auto const bytes = std::size(range) * sizeof(*std::data(range));
            std::memcpy(
                rcast<std::byte*>(std::data(dest)) + offset.get(),
                rcast<const std::byte*>(std::data(range)),
                static_cast<usize>(bytes)
            );
        }

        template<writable_buffer Dest, contiguous_buffer R>
        STX_FORCE_INLINE
        void write( Dest&& dest, const R& range ) noexcept
        {
            write(std::forward<Dest>(dest), off_s{0}, range);
        }

        // ENDIAN-AWARE WRITE ----------------------------------------------------
        template<byte_swappable Type, address_like Addr>
        STX_FORCE_INLINE
        void write_le( Addr base, Type value ) noexcept
        {
            std::memcpy(
                rcast<std::byte*>(normalize_addr(base)),
                &value,
                sizeof(Type)
            );
        }

        template<byte_swappable Type, address_like Addr>
        STX_FORCE_INLINE
        void write_be( Addr base, Type value ) noexcept
        {
            using Raw = details::raw_for_endian<Type>::type;
            auto raw = static_cast<Raw>(value);
            if constexpr ( std::endian::native == std::endian::little )
                raw = std::byteswap(raw);
            std::memcpy(
                rcast<std::byte*>(normalize_addr(base)),
                &raw,
                sizeof(Raw)
            );
        }

        // UNSAFE MEMORY ACCESS (direct deref, requires alignment, strict-aliasing) --
        template<binary_readable Type, address_like Addr>
        [[nodiscard]] STX_FORCE_INLINE Type read_raw( Addr base ) noexcept
        {
            auto* target_ptr = rcast<std::byte*>(normalize_addr(base));

            #if defined(__cpp_lib_start_lifetime_as)
                return *std::start_lifetime_as<Type>(target_ptr);
            #else
                return *reinterpret_cast<const Type*>(target_ptr);
            #endif
        }

        template<binary_readable Type, address_like Addr>
        STX_FORCE_INLINE
        void write_raw( Addr base, Type value ) noexcept
        {
            *rcast<Type*>( rcast<std::byte*>(normalize_addr(base)) ) = value;
        }

        // ALIGNMENT ---------------------------------------------------------
        template<std::unsigned_integral T>
        [[nodiscard]] constexpr T align_up( T value, T alignment ) noexcept {
            return ( value + alignment - 1 ) & ~( alignment - 1 );
        }

        template<std::unsigned_integral T>
        [[nodiscard]] constexpr T align_down( T value, T alignment ) noexcept {
            return value & ~( alignment - 1 );
        }

        template<typename T, typename Tag, std::integral U>
        [[nodiscard]] STX_FORCE_INLINE
        constexpr auto align_up(details::strong_type<T, Tag> st, U alignment) noexcept {
            using UT = std::make_unsigned_t<T>;
            return details::strong_type<T, Tag>{
                static_cast<T>(align_up(static_cast<UT>(st.get()), static_cast<UT>(alignment)))
            };
        }

        template<typename T, typename Tag, std::integral U>
        [[nodiscard]] STX_FORCE_INLINE
        constexpr auto align_down(details::strong_type<T, Tag> st, U alignment) noexcept {
            using UT = std::make_unsigned_t<T>;
            return details::strong_type<T, Tag>{
                static_cast<T>(align_down(static_cast<UT>(st.get()), static_cast<UT>(alignment)))
            };
        }

        template<typename... Args>
        inline constexpr off_s gap_v = off_s{( sizeof(Args) + ... )};

        template<usize Align, typename... Args>
        inline constexpr off_s gap_align_v = [] {
            usize total = 0;
            auto accumulate = [&total]<typename T>() {
                total = align_up(total, alignof(T));
                total += sizeof(T);
            };

            ( accumulate.template operator()<Args>(), ... );
            return off_s( align_up( total, Align ));
        }();

    } // namespace mem


    template<typename T>
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
        // Single integral: element-level,  ptr[n] = addr + n * sizeof(T)
        // Byte-offset:     byte-level,     ptr[off_s{n}] = addr + n  (no * sizeof)
        // Two integrals:    custom step,    ptr[n, s] = addr + n * s
        // ref-qualified:    p[n][m] is deleted (temporary [] disallowed)

        template<std::integral U>
        [[nodiscard]] constexpr ptr<T> operator[]( U offset ) const & noexcept {
            if constexpr ( std::is_void_v<T> )
                return ptr<T>( address + static_cast<::lbyte::stx::uptr>( offset ));
            else
                return ptr<T>( address + static_cast<::lbyte::stx::uptr>( offset ) * sizeof(T) );
        }

        template<std::integral U>
        [[nodiscard]] constexpr ptr<T> operator[]( U offset ) const && = delete;

        // Byte-offset types (off_s / rva_s): byte-level, ptr[off] = addr + off (no * sizeof)
        template<byte_offset O>
        [[nodiscard]] constexpr ptr<T> operator[]( O offset ) const & noexcept {
            return ptr<T>( address + static_cast<::lbyte::stx::uptr>( offset.get() ));
        }

        template<byte_offset O>
        [[nodiscard]] constexpr ptr<T> operator[]( O offset ) const && = delete;

        template<std::integral U, std::integral V>
        [[nodiscard]] constexpr ptr<T> operator[]( U offset, V step ) const & noexcept {
            return ptr<T>( address + static_cast<::lbyte::stx::uptr>( offset ) * static_cast<::lbyte::stx::uptr>( step ));
        }

        template<std::integral U, std::integral V>
        [[nodiscard]] constexpr ptr<T> operator[]( U offset, V step ) const && = delete;

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
            return ptr( mem::align_up( address, static_cast<usize>(alignment) ));
        }

        template<std::unsigned_integral U = usize>
        [[nodiscard]] constexpr ptr align_down( U alignment ) const noexcept {
            return ptr( mem::align_down( address, static_cast<usize>(alignment) ));
        }

        // ---- INCREMENT / DECREMENT --------------------------------
        // Element-level: advances by sizeof(T) (or 1 if void).

        constexpr ptr& operator++() noexcept {
            if constexpr ( std::is_void_v<T> )
                address += 1;
            else
                address += sizeof(T);

            return *this;
        }

        constexpr ptr operator++(int) noexcept {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        constexpr ptr& operator--() noexcept {
            if constexpr ( std::is_void_v<T> )
                address -= 1;
            else
                address -= sizeof(T);

            return *this;
        }

        constexpr ptr operator--(int) noexcept {
            auto tmp = *this;
            --(*this);
            return tmp;
        }

        // ---- ARITHMETIC -------------------------------------------
        // Only byte_offset types (off_s / rva_s) — no raw integral arithmetic.

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
        // Byte-level (off_s or off_s-convertible). No stride involved.

        template<typename ReturnType = T, byte_offset OffT>
        [[nodiscard]] STX_FORCE_INLINE
        auto walk( OffT offset ) const noexcept -> ptr<ReturnType>
            requires ( not std::is_void_v<ReturnType> )
        {
            ::lbyte::stx::uptr target = address + static_cast<::lbyte::stx::uptr>( offset.get() );
            ReturnType value;
            std::memcpy( &value, rcast<const std::byte*>( target ), sizeof( ReturnType ));
            return ptr<ReturnType>( rcast<ReturnType*>( value ) );
        }

        template<typename ReturnType = T, byte_offset OffT>
        [[nodiscard]] STX_FORCE_INLINE
        auto walk( OffT offset ) const noexcept -> ptr<void>
            requires ( std::is_void_v<ReturnType> )
        {
            ::lbyte::stx::uptr target = address + static_cast<::lbyte::stx::uptr>( offset.get() );
            ::lbyte::stx::uptr value;
            std::memcpy( &value, rcast<const std::byte*>( target ), sizeof( ::lbyte::stx::uptr ));
            return ptr<void>( value );
        }

        template<typename ReturnType = T, std::integral U>
        [[nodiscard]] STX_FORCE_INLINE
        auto walk( U offset ) const noexcept {
            return walk<ReturnType>( off_s{ offset } );
        }

        // ---- CHAIN: base / off / off -----------------------------
        // Byte-level: operator>> reads a uptr from address + offset.get()
        // (no sizeof(T), no stride). Integral overload wraps to off_s.

        template<byte_offset OffT>
        [[nodiscard]] STX_FORCE_INLINE
        auto operator>>( OffT offset ) const noexcept -> ptr<T>
        {
            ::lbyte::stx::uptr target = address + static_cast<::lbyte::stx::uptr>( offset.get() );
            ::lbyte::stx::uptr value;
            std::memcpy( &value, rcast<const std::byte*>( target ), sizeof( ::lbyte::stx::uptr ));
            return ptr<T>( rcast<T*>( value ) );
        }

        template<std::integral U>
        [[nodiscard]] STX_FORCE_INLINE
        auto operator>>( U offset ) const noexcept {
            return operator>>( off_s{ offset } );
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

#include <format>
#include <functional>

template<typename T>
struct std::hash<lbyte::stx::ptr<T>>
{
    [[nodiscard]] auto operator()( const lbyte::stx::ptr<T>& p ) const noexcept {
        return std::hash<lbyte::stx::uptr>{}( p.addr() );
    }
};

template<typename T>
struct std::formatter<lbyte::stx::ptr<T>> : std::formatter<void*> {
    auto format(const lbyte::stx::ptr<T>& p, format_context& ctx) const {
        return std::formatter<void*>::format(reinterpret_cast<void*>(p.addr()), ctx);
    }
};

#undef STX_FORCE_INLINE
