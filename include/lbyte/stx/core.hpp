#pragma once

#if defined(__GNUC__) || defined(__clang__)
    #define STX_FORCE_INLINE [[gnu::always_inline]] inline
#else
    #define STX_FORCE_INLINE inline
#endif

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace lbyte::stx
{
    struct version_info { int major, minor, patch; };
    inline constexpr version_info version { 2, 0, 0 };

    using u8    = std::uint8_t  ;
    using u16   = std::uint16_t ;
    using u32   = std::uint32_t ;
    using u64   = std::uint64_t ;

    using i8    = std::int8_t   ;
    using i16   = std::int16_t  ;
    using i32   = std::int32_t  ;
    using i64   = std::int64_t  ;

    using f32   = float         ;
    using f64   = double        ;

    using isize = std::ptrdiff_t;
    using usize = std::size_t   ;

    using uptr  = std::uintptr_t;
    using iptr  = std::intptr_t ;

    namespace details
    {
        struct offset_tag {};
        struct rva_tag    {};
        struct va_tag     {};

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

                template<typename Self>
                [[nodiscard]] constexpr auto&& get( this Self&& self ) noexcept {
                    return std::forward<Self>( self ).value;
                }

                template<typename Self>
                constexpr explicit operator Type( this Self&& self ) noexcept {
                    return std::forward<Self>( self ).value;
                }

                constexpr strong_type& operator+=( Type rhs ) noexcept {
                    value += rhs;
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

                friend constexpr auto
                operator<=>(const strong_type&, const strong_type&) = default;

                template<typename T>
                struct effective_size {
                    static constexpr usize value = sizeof(T);
                };

                template<typename T, typename TTag>
                struct effective_size<strong_type<T, TTag>> {
                    static constexpr usize value = sizeof(T);
                };

            private:
                Type value{};
        };
    }

    using off_s = details::strong_type<usize, details::offset_tag>;
    using rva_s = details::strong_type<u32  , details::rva_tag   >;
    using va_s  = details::strong_type<uptr , details::va_tag    >;

    enum class origin : u8
    {
        begin  ,
        current,
        end    ,
    };

    // ALIGNMENT -------------------------------------------------------------
    template<std::unsigned_integral T>
    [[nodiscard]] constexpr T align_up( T value, T alignment ) noexcept {
        return ( value + alignment - 1 ) & ~( alignment - 1 );
    }

    template<std::unsigned_integral T>
    [[nodiscard]] constexpr T align_down( T value, T alignment ) noexcept {
        return value & ~( alignment - 1 );
    }

    template<typename T, typename Tag, typename U>
    [[nodiscard]] STX_FORCE_INLINE
    constexpr auto align_up(details::strong_type<T, Tag> st, U alignment) noexcept {
        return details::strong_type<T, Tag>{ align_up(st.get(), static_cast<T>(alignment)) };
    }

    template<typename T, typename Tag, typename U>
    [[nodiscard]] STX_FORCE_INLINE
    constexpr auto align_down(details::strong_type<T, Tag> st, U alignment) noexcept {
        return details::strong_type<T, Tag>{ align_down(st.get(), static_cast<T>(alignment)) };
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

    template<typename Type>
    concept address_like
        =  std::is_pointer_v<Type>
        or std::same_as<std::remove_cv_t   <Type>, std::uintptr_t>
        or std::same_as<std::remove_cv_t   <Type>, std::intptr_t >
        or std::same_as<std::remove_cvref_t<Type>, va_s          >;

    template<class Type>
    concept binary_readable
        =       std::is_trivially_copyable_v<Type>
        and     std::is_standard_layout_v   <Type>
        and not std::is_empty_v             <Type>
        and not std::is_pointer_v           <Type>;

    template<address_like Addr> [[nodiscard]]
    constexpr uptr normalize_addr( Addr base ) noexcept
    {
        if constexpr ( std::is_pointer_v<Addr> )
            return reinterpret_cast<uptr>( base );
        else if constexpr ( std::same_as<std::remove_cvref_t<Addr>, va_s> )
            return static_cast<uptr>( base.get() );
        else
            return static_cast<uptr>( base );
    }
}

#undef STX_FORCE_INLINE

