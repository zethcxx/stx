#pragma once
#include "core.hpp"
#include "arr.hpp"
#include "fn.hpp"
#include <bit>
#include <compare>
#include <cstring>
#include <functional>
#include <memory>
#include <span>
#include <string_view>
#include <tuple>

#if defined(__GNUC__) || defined(__clang__)
    #define STX_FORCE_INLINE [[gnu::always_inline]] inline
#else
    #define STX_FORCE_INLINE inline
#endif

namespace lbyte::stx
{
    template<typename T> class ptr;   // fwd (used by ptr_ops and walk)

    namespace details {
        template<typename T>
        struct raw_for_endian { using type = T; };

        template<typename T> requires std::is_enum_v<T>
        struct raw_for_endian<T> { using type = std::underlying_type_t<T>; };

        // --- scalar reader ---------------------------------------------------
        // Pure scalar fast path (memcpy, well-defined, unaligned-safe).
        // Fixed-size arrays use the dedicated bounded_array read/pop overloads,
        // so `reader_of` deliberately has no read() here.

        template<typename U>
        struct reader_of {
            using value_t = U;
            static constexpr usize bytes_count = sizeof(U);
            static constexpr value_t read( const uptr addr ) noexcept
                requires ( binary_readable<U> and not std::is_array_v<U> )
            {
                value_t value;
                std::memcpy( &value, rcast<const std::byte*>(addr), sizeof(U) );
                return value;
            }
        };

        template<typename T, usize N>
        struct reader_of<T[N]>
        {
            using value_t = T[N];
            static constexpr usize bytes_count = sizeof(T[N]);
        };

        // --- record reader fingerprint ---------------------------------------
        // A `ct::record<...>` (or any type) is readable through the generic
        // ptr/memcur ops as soon as it exposes a nested `reader`:
        //
        //     struct reader {
        //         using value_t      = ...;
        //         static constexpr usize byte_size = ...;
        //         static constexpr value_t read(uptr addr) noexcept;
        //     };
        //
        // include/lbyte/stx/ct/record.hpp provides one via its `record_ops`
        // base (`ct::record<...>::reader`), so mem.hpp never needs to name the
        // record type nor specialize anything.

        template<typename U>
        concept record_like = requires {
            typename U::reader::value_t;
            { U::reader::read( ::lbyte::stx::uptr{} ) }
                -> std::same_as<typename U::reader::value_t>;
        };

        template<record_like U>
        [[nodiscard]] constexpr auto read_value( const ::lbyte::stx::uptr addr ) noexcept
        {
            return U::reader::read( addr );
        }

        template<typename U>
            requires ( binary_readable<U> && not std::is_array_v<U> )
        [[nodiscard]] constexpr U read_value( const ::lbyte::stx::uptr addr ) noexcept
        {
            U value;
            std::memcpy( &value, rcast<const std::byte*>(addr), sizeof(U) );
            return value;
        }

        template<typename U>
        struct byte_count_t
        {
            static constexpr ::lbyte::stx::usize value = sizeof(U);
        };

        template<record_like U>
        struct byte_count_t<U>
        {
            static constexpr ::lbyte::stx::usize value = U::reader::byte_size;
        };

        template<typename U>
        constexpr ::lbyte::stx::usize byte_count_of = byte_count_t<U>::value;

        // --- generic ptr ops (CRTP) ------------------------------------------
        // The read family shared by `ptr<T>` and `memcur<ByteType>` lives here
        // so new ops do not have to be added to those class bodies. The derived
        // type must expose:
        //   * `addr() const`      (current read address)
        //   * `advance_bytes(n)`  (advance pop position by n bytes)
        // `ValueT` is passed explicitly from the derived type (e.g. ...ptr<T>, T>)
        // so the mixin never has to name the still-incomplete derived type.

        template<typename Self, typename ValueT>
        class ptr_ops
        {
            [[nodiscard]] constexpr Self& self() noexcept { return static_cast<Self&>(*this); }
            [[nodiscard]] constexpr const Self& self() const noexcept { return static_cast<const Self&>(*this); }

        public:
            // stateless base: lets derived types default `operator==`/cmp
            constexpr bool operator==( const ptr_ops& ) const noexcept = default;
            constexpr auto operator<=>( const ptr_ops& ) const noexcept = default;

            // ---- READ (scalar or record, no advance) ------------------------
            // Every mixin member uses a trailing return type that is SFINAE-safe
            // under eager instantiation (GCC substitutes the default U while the
            // derived type is still incomplete), so signatures never touch the
            // derived type - only the bodies do, and those are lazy.

            template<typename U = ValueT>
            [[nodiscard]] STX_FORCE_INLINE
            auto read() const noexcept
                -> decltype( read_value<U>( ::lbyte::stx::uptr{} ) )
                requires ( not std::is_array_v<U> && ( binary_readable<U> || record_like<U> ) )
            {
                return read_value<U>( self().addr() );
            }

            template<bounded_array U>
            [[nodiscard]] STX_FORCE_INLINE
            auto read() const noexcept -> bounded_array_t<U>
            {
                bounded_array_t<U> arr;
                std::memcpy( &arr, rcast<const std::byte*>( self().addr() ), sizeof(arr) );
                return arr;
            }

            // ---- READ AS std::array (copy, no advance) ----------------------
            // Reads N elements as a std::array<U, N>. Two forms:
            //   read_array<U[N]>()          N deduced from the C-array bound
            //   read_array<U, N>()          N explicit as a template parameter

            template<bounded_array U>
            [[nodiscard]] STX_FORCE_INLINE
            auto read_array() const noexcept -> bounded_array_t<U>
            {
                bounded_array_t<U> raw{};
                std::memcpy( &raw, rcast<const std::byte*>( self().addr() ), sizeof(raw) );
                return raw;
            }

            template<typename U = ValueT, usize N>
                requires ( not std::is_void_v<U> && binary_readable<std::remove_cv_t<U>> )
            [[nodiscard]] STX_FORCE_INLINE
            auto read_array() const noexcept -> std::array<std::remove_cv_t<U>, N>
            {
                using elem = std::remove_cv_t<U>;
                std::array<elem, N> raw{};
                std::memcpy( &raw, rcast<const std::byte*>( self().addr() ), sizeof(raw) );
                return raw;
            }

            // ---- READ AS stx::arr (copy, no advance) ------------------------
            // Fixed-size, value-semantic array type. Same two forms as
            // read_array<...>, but returns `stx::arr<elem, N>`.

            template<bounded_array U>
            [[nodiscard]] STX_FORCE_INLINE
            auto read_arr() const noexcept
                -> stx::arr<std::remove_cv_t<std::remove_all_extents_t<U>>,
                            std::tuple_size_v<bounded_array_t<U>>>
            {
                using flat = bounded_array_t<U>;
                using elem = std::remove_cv_t<std::remove_all_extents_t<U>>;
                std::array<elem, std::tuple_size_v<flat>> raw{};
                std::memcpy( &raw, rcast<const std::byte*>( self().addr() ), sizeof(raw) );
                return stx::arr<elem, std::tuple_size_v<flat>>{ raw };
            }

            template<typename U = ValueT, usize N>
                requires ( not std::is_void_v<U> && binary_readable<std::remove_cv_t<U>> )
            [[nodiscard]] STX_FORCE_INLINE
            auto read_arr() const noexcept -> stx::arr<std::remove_cv_t<U>, N>
            {
                using elem = std::remove_cv_t<U>;
                std::array<elem, N> raw{};
                std::memcpy( &raw, rcast<const std::byte*>( self().addr() ), sizeof(raw) );
                return stx::arr<elem, N>{ raw };
            }

            // ---- READ AS ptr<U> (pointer-sized, no advance) ------------------

            template<typename U = ValueT>
            [[nodiscard]] STX_FORCE_INLINE
            auto read_p() const noexcept -> ptr<U>
                requires ( not std::is_void_v<U> )
            {
                ::lbyte::stx::uptr value;
                std::memcpy(
                    &value,
                    rcast<const std::byte*>( self().addr() ),
                    sizeof(::lbyte::stx::uptr)
                );
                return ptr<U>( rcast<U*>( value ) );
            }

            // ---- POP (read + advance) --------------------------------------

            template<typename U = ValueT>
            [[nodiscard]] STX_FORCE_INLINE
            auto pop() noexcept
                -> decltype( read_value<U>( ::lbyte::stx::uptr{} ) )
                requires ( not std::is_array_v<U> && ( binary_readable<U> || record_like<U> ) )
            {
                auto value = read_value<U>( self().addr() );
                self().advance_bytes( byte_count_of<U> );
                return value;
            }

            template<bounded_array U>
            [[nodiscard]] STX_FORCE_INLINE
            auto pop() noexcept -> bounded_array_t<U>
            {
                bounded_array_t<U> arr;
                std::memcpy( &arr, rcast<const std::byte*>( self().addr() ), sizeof(arr) );
                self().advance_bytes( sizeof(arr) );
                return arr;
            }
        };
    }

    namespace mem {

        // SAFE MEMORY ACCESS (memcpy, well-defined, unaligned-safe) -----------------
        template<binary_readable Type, address_like Addr>
        [[nodiscard]] STX_FORCE_INLINE
        Type read( Addr base ) noexcept
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
        [[nodiscard]] STX_FORCE_INLINE Type read_be( Addr base ) noexcept
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
        constexpr auto align_up(newtype<T, Tag> st, U alignment) noexcept {
            using UT = std::make_unsigned_t<T>;
            return newtype<T, Tag>{
                static_cast<T>(align_up(static_cast<UT>(st.get()), static_cast<UT>(alignment)))
            };
        }

        template<typename T, typename Tag, std::integral U>
        [[nodiscard]] STX_FORCE_INLINE
        constexpr auto align_down(newtype<T, Tag> st, U alignment) noexcept {
            using UT = std::make_unsigned_t<T>;
            return newtype<T, Tag>{
                static_cast<T>(align_down(static_cast<UT>(st.get()), static_cast<UT>(alignment)))
            };
        }

        // DIFF -------------------------------------------------------------------
        template<address_like A, address_like B>
        [[nodiscard]] constexpr off_s diff(A a, B b) noexcept
        {
            return off_s{ scast<off_s::value_type>(
                normalize_addr(a) - normalize_addr(b)
            )};
        }

        template<std::integral T, address_like A, address_like B>
        [[nodiscard]] constexpr T diff(A a, B b) noexcept
        {
            return scast<T>(normalize_addr(a) - normalize_addr(b));
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


    template<typename T, typename = void>
    struct ptr_char { using type = std::remove_cv_t<T>; };
    template<typename T>
    struct ptr_char<T, std::enable_if_t<std::is_void_v<std::remove_cv_t<T>>>> { using type = char; };

    template<typename T>
    class ptr : public details::ptr_ops<ptr<T>, T>
    {
        ::lbyte::stx::uptr address = 0;

    public:
        using value_type = T;
        using char_type = typename ptr_char<T>::type;
        using view_type = std::basic_string_view<char_type>;

        constexpr ptr() noexcept = default;

        [[nodiscard]] constexpr ptr(T* raw_ptr) noexcept
          : address { rcast<::lbyte::stx::uptr>(raw_ptr) }
        {}

        constexpr ptr(address_like auto addr) noexcept
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

        // mixin hook (details::ptr_ops): advance pop position by n raw bytes
        constexpr void advance_bytes( const usize n ) noexcept {
            address += n;
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

        // Element-level comparison: compares sizeof(T) bytes of pointed-to
        // content against a T value. Only for non-void element types and
        // non-ptr values; the address-level `operator==(const ptr&)` above
        // still compares locations.
        template<typename U = T>
            requires ( not std::is_void_v<U>
                   and not std::same_as<std::remove_cvref_t<U>, ptr> )
        [[nodiscard]] constexpr bool operator==( const U& value ) const noexcept {
            return *rcast<const U*>(address) == value;
        }

        template<typename U = T>
            requires ( not std::is_void_v<U>
                   and not std::same_as<std::remove_cvref_t<U>, ptr> )
        [[nodiscard]] constexpr bool operator!=( const U& value ) const noexcept {
            return !( *this == value );
        }

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
        // read/read_array/read_arr/read_p/pop live in details::ptr_ops below.

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
            requires ( not std::is_void_v<U> and binary_readable<U> and not contiguous_buffer<U> )
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
            requires ( not std::is_void_v<U> and binary_readable<U> and not contiguous_buffer<U> )
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
        {
            using base = std::remove_cv_t<std::remove_all_extents_t<U>>;
            using elem = std::conditional_t<
                (std::is_const_v<std::remove_all_extents_t<U>> || std::is_const_v<T>),
                const base, base>;
            using flat_array = bounded_array_t<U>;
            return std::span<elem>(
                rcast<elem*>(address),
                sizeof(flat_array) / sizeof(base)
            );
        }

        template<typename U = T>
            requires ( not std::is_void_v<U> && binary_readable<std::remove_cv_t<U>> )
        [[nodiscard]] STX_FORCE_INLINE
        auto as_view( usize count ) const noexcept
        {
            using base = std::remove_cv_t<U>;
            using elem = std::conditional_t<
                (std::is_const_v<U> || std::is_const_v<T>), const base, base>;
            return std::span<elem>( rcast<elem*>( address ), count );
        }

        // ---- CONTENT COMPARE (no advance) -------------------------
        // Semantics like std::memcmp: returns 0 if equal, <0 if this is
        // lexicographically "less", >0 if "greater". In a boolean context
        // `if (p.cmp(...))` is true when the buffers differ.

        [[nodiscard]] STX_FORCE_INLINE
        int cmp( const void* data, usize len ) const noexcept
        {
            return std::memcmp( rcast<const void*>(address), data, len );
        }

        template<contiguous_buffer R>
        [[nodiscard]] STX_FORCE_INLINE
        int cmp( const R& range ) const noexcept
        {
            auto const bytes = std::size(range) * sizeof(*std::data(range));
            return std::memcmp(
                rcast<const void*>(address),
                std::data(range),
                static_cast<usize>(bytes)
            );
        }

        // Compare against a scalar value's bytes (e.g. ct::istr<"...">).
        template<std::integral U>
        [[nodiscard]] STX_FORCE_INLINE
        int cmp( const U value ) const noexcept
        {
            U v = value;
            return std::memcmp( rcast<const void*>(address), &v, sizeof(U) );
        }

        // ---- CONTENT EQUALITY (bool, no advance) ------------------
        // `eq` is the readable boolean form of `cmp(...) == 0`; the element
        // type is deduced (or explicit, e.g. eq<u32>) from the argument.
        // `if (base[off].eq(var))` reads naturally, unlike the memcmp-style
        // int of cmp. cmp() remains for those needing the <-/-> ordering.

        [[nodiscard]] STX_FORCE_INLINE
        bool eq( const void* data, usize len ) const noexcept
        {
            return std::memcmp( rcast<const void*>(address), data, len ) == 0;
        }

        template<contiguous_buffer R>
        [[nodiscard]] STX_FORCE_INLINE
        bool eq( const R& range ) const noexcept
        {
            auto const bytes = std::size(range) * sizeof(*std::data(range));
            return std::memcmp(
                rcast<const void*>(address),
                std::data(range),
                static_cast<usize>(bytes)
            ) == 0;
        }

        template<std::integral U>
        [[nodiscard]] STX_FORCE_INLINE
        bool eq( const U value ) const noexcept
        {
            U v = value;
            return std::memcmp( rcast<const void*>(address), &v, sizeof(U) ) == 0;
        }

        // ---- STRING VIEW (zero-copy) ------------------------------
        // Interpret the pointed-to bytes as a character string. The element
        // type determines the view's char type (ptr<char> -> string_view,
        // ptr<wchar_t> -> wstring_view, ptr<void> -> string_view).

        // Null-terminated: scans until the first 0 (expensive; bounded by
        // the actual string length). Returns a view of the content.
        [[nodiscard]] STX_FORCE_INLINE
        view_type read_strv() const noexcept
        {
            const char_type* p = rcast<const char_type*>(address);
            return view_type( p, std::char_traits<char_type>::length( p ));
        }

        // Sized: zero-copy view of exactly count elements. No scan.
        [[nodiscard]] STX_FORCE_INLINE
        view_type read_strv( usize count ) const noexcept
        {
            return view_type( rcast<const char_type*>(address), count );
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
        // Only byte_offset types (off_s / rva_s) - no raw integral arithmetic.

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

        template<std::integral Ret, address_like Addr>
        [[nodiscard]] constexpr Ret diff( Addr other ) const noexcept {
            return scast<Ret>( address - normalize_addr(other) );
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

#ifndef STX_MODULE_BUILD

#include "detail/ptr_support.hpp"

#endif

#undef STX_FORCE_INLINE
