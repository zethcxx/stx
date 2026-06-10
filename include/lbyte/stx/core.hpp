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
#include <type_traits>
#include <utility>

namespace lbyte::stx
{
    struct version_info { int major, minor, patch; };
    inline constexpr version_info version { 1, 0, 0 };

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

        template<typename Tag>
        struct is_offset_tag : std::false_type {};
        template<> struct is_offset_tag<offset_tag> : std::true_type {};
        template<> struct is_offset_tag<rva_tag>    : std::true_type {};

        template< typename Type, typename Tag >
        class strong_type
        {
            public:
                using value_type = Type;
                using tag_type   = Tag ;

                constexpr strong_type() noexcept = default;
                constexpr explicit strong_type( value_type _value ) noexcept
                    : value { _value }
                {}

                template<std::integral U>
                constexpr explicit strong_type( U v ) noexcept
                    : value( static_cast<value_type>( v ) )
                {}

                template<typename U, typename Tag2>
                    requires (is_offset_tag<Tag>::value && is_offset_tag<Tag2>::value)
                constexpr explicit strong_type( strong_type<U, Tag2> other ) noexcept
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

                constexpr strong_type& operator++()    noexcept { ++value; return *this; }
                constexpr strong_type  operator++(int) noexcept { auto t{*this}; ++value; return t; }
                constexpr strong_type& operator--()    noexcept { --value; return *this; }
                constexpr strong_type  operator--(int) noexcept { auto t{*this}; --value; return t; }

                constexpr strong_type& operator+=( Type rhs ) noexcept {
                    value += rhs;
                    return *this;
                }
                constexpr strong_type& operator+=( strong_type rhs ) noexcept {
                    value += rhs.value;
                    return *this;
                }

                friend constexpr strong_type operator+( strong_type lhs, Type rhs ) noexcept {
                    lhs.value += rhs;
                    return lhs;
                }

                friend constexpr strong_type operator+( strong_type lhs, strong_type rhs ) noexcept {
                    return lhs += rhs.value;
                }

                friend constexpr Type operator+( Type lhs, strong_type rhs ) noexcept {
                    return lhs + rhs.value ;
                }

                constexpr strong_type& operator-=( Type rhs ) noexcept {
                    value -= rhs;
                    return *this;
                }
                constexpr strong_type& operator-=( strong_type rhs ) noexcept {
                    value -= rhs.value;
                    return *this;
                }

                friend constexpr Type operator-( strong_type lhs, strong_type rhs ) noexcept {
                    return lhs.get() - rhs.get();
                }

                friend constexpr strong_type operator-( strong_type lhs, Type rhs ) noexcept {
                    lhs.value -= rhs;
                    return lhs;
                }

                friend constexpr Type operator-( Type lhs, strong_type rhs ) noexcept {
                    return lhs - rhs.value;
                }

                friend constexpr strong_type operator-( strong_type value ) noexcept {
                    return strong_type{ -value.value };
                }

                friend constexpr auto
                operator<=>(const strong_type&, const strong_type&) = default;

            private:
                Type value{};
        };
    }

    using off_s = details::strong_type<std::ptrdiff_t, details::offset_tag>;
    using rva_s = details::strong_type<u32  , details::rva_tag   >;
    using va_s  = details::strong_type<uptr , details::va_tag    >;

    template<typename... Args>
    inline constexpr off_s gap_v = off_s{( sizeof(Args) + ... )};

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
        template<typename T>
        using bounded_array_t = typename bounded_array_impl<T>::type;
    }

    template<typename T>
    concept byte_offset
        =  std::same_as<std::remove_cvref_t<T>, off_s>
        or std::same_as<std::remove_cvref_t<T>, rva_s>;

    namespace details {
        template<typename T, usize... Dims>
        struct nested_array;

        template<typename T, usize N>
        struct nested_array<T, N> {
            using type = std::array<T, N>;
        };

        template<typename T, usize First, usize... Rest>
        struct nested_array<T, First, Rest...> {
            using type = std::array<typename nested_array<T, Rest...>::type, First>;
        };

        template<typename T, usize... Dims>
        using nested_array_t = typename nested_array<T, Dims...>::type;
    }

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
    STX_FORCE_INLINE
    Type ccast( auto value ) noexcept {
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
        ~defer() { if (armed_) fn_(); }
        void cancel() noexcept { armed_ = false; }
    };
    template<std::invocable<> F> defer(F) -> defer<F>;

    // --- null_t ----------------------------------------------------------------

    struct null_t {
        constexpr operator uptr()         const noexcept { return 0; }
        constexpr operator std::nullptr_t() const noexcept { return nullptr; }
        constexpr explicit operator bool() const noexcept { return false; }

        friend constexpr const null_t& operator<<(const null_t& n, auto&&) noexcept { return n; }
    };

    inline constexpr null_t null{};
}

#undef STX_FORCE_INLINE

