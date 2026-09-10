#pragma once

#if defined(__GNUC__) || defined(__clang__)
    #define STX_FORCE_INLINE [[gnu::always_inline]] inline
#else
    #define STX_FORCE_INLINE inline
#endif

#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <type_traits>
#include <utility>

namespace lbyte::stx
{
    struct version_info { int major, minor, patch; };
    inline constexpr version_info version { 0, 3, 0 };

    using u8        = std::uint8_t      ;
    using u16       = std::uint16_t     ;
    using u32       = std::uint32_t     ;
    using u64       = std::uint64_t     ;

    using i8        = std::int8_t       ;
    using i16       = std::int16_t      ;
    using i32       = std::int32_t      ;
    using i64       = std::int64_t      ;

    using f32       = float             ;
    using f64       = double            ;

    using uchar     = unsigned char     ;
    using ushort    = unsigned short    ;
    using uint      = unsigned int      ;
    using ulong     = unsigned long     ;
    using ulonglong = unsigned long long;

    using isize     = std::ptrdiff_t    ;
    using usize     = std::size_t       ;

    using uptr      = std::uintptr_t    ;
    using iptr      = std::intptr_t     ;

    namespace details
    {
        struct offset_tag {};
        struct rva_tag    {};
        struct va_tag     {};
    }

    // Hook (extensible): a tag is "offset-like" when it can be added to /
    // subtracted from addresses like a byte offset. Built-in tags are
    // offset_tag (off_s) and rva_tag (rva_s); user tags may specialize this
    // trait to opt into byte_offset behaviour.
    template<typename Tag>
    struct is_offset_tag : std::false_type {};
    template<> struct is_offset_tag<details::offset_tag> : std::true_type {};
    template<> struct is_offset_tag<details::rva_tag>    : std::true_type {};

    // Public newtype: a distinct type with the same runtime representation as
    // its backing `Type`, discriminated by `Tag`. Allows external projects to
    // define their own strong types (e.g. off32_s) without editing this header.
    template< typename Type, typename Tag >
    class newtype
    {
            public:
                using value_type = Type;
                using tag_type   = Tag ;

                constexpr newtype() noexcept = default;
                constexpr explicit newtype( value_type _value ) noexcept
                    : value { _value }
                {}

                template<std::integral U>
                constexpr explicit newtype( U v ) noexcept
                    : value( static_cast<value_type>( v ) )
                {}

                template<typename U, typename Tag2>
                    requires (is_offset_tag<Tag>::value && is_offset_tag<Tag2>::value)
                constexpr explicit newtype( newtype<U, Tag2> other ) noexcept
                    : value( static_cast<value_type>( other.get() ) )
                {}

                template<typename Self>
                [[nodiscard]] constexpr auto&& get( this Self&& self ) noexcept {
                    return std::forward<Self>( self ).value;
                }

                template<typename Self>
                constexpr explicit operator Type( this Self&& self ) noexcept {
                    return std::forward<Self>( self ).value;
                }

                template<typename U>
                [[nodiscard]] constexpr auto as() const noexcept -> U {
                    return static_cast<U>( get() );
                }

                constexpr newtype& operator++()    noexcept { ++value; return *this; }
                constexpr newtype  operator++(int) noexcept { auto t{*this}; ++value; return t; }
                constexpr newtype& operator--()    noexcept { --value; return *this; }
                constexpr newtype  operator--(int) noexcept { auto t{*this}; --value; return t; }

                constexpr newtype& operator+=( Type rhs ) noexcept {
                    value += rhs;
                    return *this;
                }
                constexpr newtype& operator+=( newtype rhs ) noexcept {
                    value += rhs.value;
                    return *this;
                }
                template<typename T2, typename Tag2>
                    requires ( not std::same_as<Tag, Tag2> )
                constexpr newtype& operator+=( newtype<T2, Tag2> rhs ) noexcept {
                    value += static_cast<Type>( rhs.get() );
                    return *this;
                }

                friend constexpr newtype operator+( newtype lhs, Type rhs ) noexcept {
                    lhs.value += rhs;
                    return lhs;
                }

                friend constexpr newtype operator+( newtype lhs, newtype rhs ) noexcept {
                    return lhs += rhs.value;
                }

                template<typename T2, typename Tag2>
                    requires ( not std::same_as<Tag, Tag2> )
                friend constexpr newtype operator+( newtype lhs, newtype<T2, Tag2> rhs ) noexcept {
                    lhs.value += static_cast<Type>( rhs.get() );
                    return lhs;
                }

                friend constexpr Type operator+( Type lhs, newtype rhs ) noexcept {
                    return lhs + rhs.value ;
                }

                constexpr newtype& operator-=( Type rhs ) noexcept {
                    value -= rhs;
                    return *this;
                }
                constexpr newtype& operator-=( newtype rhs ) noexcept {
                    value -= rhs.value;
                    return *this;
                }
                template<typename T2, typename Tag2>
                    requires ( not std::same_as<Tag, Tag2> )
                constexpr newtype& operator-=( newtype<T2, Tag2> rhs ) noexcept {
                    value -= static_cast<Type>( rhs.get() );
                    return *this;
                }

                friend constexpr Type operator-( newtype lhs, newtype rhs ) noexcept {
                    return lhs.get() - rhs.get();
                }

                friend constexpr newtype operator-( newtype lhs, Type rhs ) noexcept {
                    lhs.value -= rhs;
                    return lhs;
                }

                friend constexpr Type operator-( Type lhs, newtype rhs ) noexcept {
                    return lhs - rhs.value;
                }

                template<typename T2, typename Tag2>
                    requires ( not std::same_as<Tag, Tag2> )
                friend constexpr newtype operator-( newtype lhs, newtype<T2, Tag2> rhs ) noexcept {
                    lhs.value -= static_cast<Type>( rhs.get() );
                    return lhs;
                }

                friend constexpr newtype operator-( newtype value ) noexcept {
                    return newtype{ -value.value };
                }

                friend constexpr auto
                operator<=>(const newtype&, const newtype&) = default;

            private:
                Type value{};
        };

    // Convenient offset-like alias: any integral backing type becomes a byte
    // offset newtype reusing the built-in offset_tag (so it is mutually
    // convertible with off_s/rva_s and works with ptr<T>[N]).
    template<typename Type>
    using offset_s = newtype<Type, details::offset_tag>;

    using off_s = newtype<std::ptrdiff_t, details::offset_tag>;
    using rva_s = newtype<u32  , details::rva_tag   >;
    using va_s  = newtype<uptr , details::va_tag    >;

    template<typename Type>
    concept address_like
        =  std::is_pointer_v<Type>
        or std::same_as<std::remove_cv_t   <Type>, std::uintptr_t>
        or std::same_as<std::remove_cv_t   <Type>, std::intptr_t >
        or std::same_as<std::remove_cvref_t<Type>, va_s          >
        or requires(Type t) { { t.addr() } -> std::same_as<uptr>; };

    template<class Type>
    concept binary_readable
        =       std::is_trivially_copyable_v<Type>
        and     std::is_standard_layout_v   <Type>
        and not std::is_empty_v             <Type>
        and not std::is_pointer_v           <Type>;

    template<typename T>
    concept byte_swappable
        =  (std::integral<T> or std::is_enum_v<T>)
        and not std::same_as<std::remove_cvref_t<T>, bool>
        and not std::same_as<std::remove_cvref_t<T>, char>
        and not std::same_as<std::remove_cvref_t<T>, wchar_t>
        and not std::same_as<std::remove_cvref_t<T>, char8_t>
        and not std::same_as<std::remove_cvref_t<T>, char16_t>
        and not std::same_as<std::remove_cvref_t<T>, char32_t>;

    template<typename R>
    concept contiguous_buffer
        =  requires (R& r) {
            { std::data(r) } -> std::convertible_to<const void*>;
            { std::size(r) } -> std::convertible_to<usize>;
           }
        && std::is_trivially_copyable_v<
               std::remove_pointer_t<decltype(std::data(std::declval<R&>()))>>;

    template<typename R>
    concept writable_buffer
        =  requires (R& r) {
            { std::data(r) } -> std::convertible_to<void*>;
            { std::size(r) } -> std::convertible_to<usize>;
           }
        && std::is_trivially_copyable_v<
               std::remove_pointer_t<decltype(std::data(std::declval<R&>()))>>;

    template<typename T>
    concept bounded_array
        =  std::is_bounded_array_v<T>
        && binary_readable<std::remove_all_extents_t<T>>;

    template<typename T>
    concept buffer_type
        =  sizeof(T) == 1
        && std::is_trivially_copyable_v<T>
        && not std::is_pointer_v<T>
        && not std::same_as<std::remove_cv_t<T>, bool>
        && not std::same_as<std::remove_cv_t<T>, void>;

    namespace details {
        template<typename T> struct bounded_array_impl { using type = T; };
        template<typename T, usize N> struct bounded_array_impl<T[N]> {
            using type = std::array<typename bounded_array_impl<T>::type, N>;
        };
    }

    template<typename T>
    using bounded_array_t = typename details::bounded_array_impl<T>::type;

    // A byte offset is any newtype whose tag is "offset-like" (see
    // is_offset_tag). Built-ins off_s/rva_s qualify; external types such as
    // off32_s qualify by using offset_s<Type> or specializing is_offset_tag.
    template<typename T>
    concept byte_offset
        =  requires { typename std::remove_cvref_t<T>::tag_type; }
        and is_offset_tag<typename std::remove_cvref_t<T>::tag_type>::value;

    template<address_like Addr> [[nodiscard]]
    constexpr uptr normalize_addr( Addr base ) noexcept
    {
        if constexpr ( std::is_pointer_v<Addr> )
            return reinterpret_cast<uptr>( base );
        else if constexpr ( std::same_as<std::remove_cvref_t<Addr>, va_s> )
            return static_cast<uptr>( base.get() );
        else if constexpr ( requires { base.addr(); } )
            return base.addr();
        else
            return static_cast<uptr>( base );
    }

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
    [[nodiscard]] STX_FORCE_INLINE
    constexpr Type ccast( auto value ) noexcept {
        return const_cast<Type>( value );
    }

    template<class Type>
    STX_FORCE_INLINE
    Type dcast( auto value ) noexcept {
        return dynamic_cast<Type>( value );
    }

    template<std::invocable<> F>
    struct defer {
        F fn_;
        bool armed_ = true;
        defer(F f) : fn_(std::move(f)) {}
        defer(defer&&) = delete;
        defer(const defer&) = delete;
        defer& operator=(defer&&) = delete;
        defer& operator=(const defer&) = delete;
        ~defer() noexcept { if (armed_) fn_(); }
        void cancel() noexcept { armed_ = false; }
    };
    template<std::invocable<> F> defer(F) -> defer<F>;

    // --- null_t ----------------------------------------------------------------

    struct null_t {
        constexpr explicit operator std::uintptr_t() const noexcept { return 0; }
        constexpr explicit operator bool() const noexcept { return false; }

        constexpr operator std::nullptr_t() const noexcept { return nullptr; }

        template<typename T>
        requires (!std::same_as<T, std::nullptr_t>)
        constexpr operator T*() const noexcept { return nullptr; }

        template<typename T>
        requires std::is_constructible_v<T, std::nullptr_t>
            && (!std::is_same_v<T, bool>)
            && (!std::is_same_v<T, std::nullptr_t>)
            && (!std::is_pointer_v<T>)
        constexpr operator T() const noexcept { return T(nullptr); }

        auto operator<=>(const null_t&) const = default;

        template<typename T> friend void operator+(null_t, T) = delete;
        template<typename T> friend void operator-(null_t, T) = delete;

        friend constexpr const null_t& operator<<(const null_t& n, auto&&) noexcept { return n; }
    };

    inline constexpr null_t null{};

    // ---- array_of factory (std::to_array style) -----------------------------
    // Deduces Type and N from a C-array: `array_of<T>({...})` and
    // `array_of({...})` both work. The lvalue overload accepts named arrays,
    // which lets C array-designators `[idx] = v` (valid only in a C-array, not
    // in a class) seed a typed std::array in a single constexpr:
    //     inline constexpr u64 raw[k] = { [3] = 9 };
    //     inline constexpr auto a = array_of( raw );   // std::array<u64, k>

    template<typename Type, usize N>
    [[nodiscard]] constexpr auto array_of( Type (&& source)[N] ) noexcept
        -> std::array<Type, N>
    {
        return std::to_array( std::move( source ) );
    }

    template<typename Type, usize N>
    [[nodiscard]] constexpr auto array_of( Type const ( &source )[N] ) noexcept
        -> std::array<Type, N>
    {
        return std::to_array( source );
    }
}

#ifndef STX_MODULE_BUILD

#include "detail/null_support.hpp"

#endif

#undef STX_FORCE_INLINE

