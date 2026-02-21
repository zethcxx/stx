#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace stx
{
    struct version_info { int major, minor, patch; };
    inline constexpr version_info version { 1, 0, 0 };

    namespace details
    {
        template< typename T, typename Tag >
        class strong_type;

        struct offset_tag {};
        struct rva_tag    {};
        struct va_tag     {};
    }

    // NORMAL TYPES --------------------------------------------------------------
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

    // STRONG TYPES --------------------------------------------------------------
    using offset_t = details::strong_type<usize, details::offset_tag>;
    using rva_t    = details::strong_type<u32  , details::rva_tag        >;
    using va_t     = details::strong_type<uptr , details::va_tag         >;

    // ALTERNATIVE TO SEEKDIR ----------------------------------------------------
    enum class origin : u8
    {
        begin  ,
        current,
        end    ,
    };

    // CONCEPTS ------------------------------------------------------------------
    template<typename Type>
    concept address_like
        =  std::is_pointer_v<Type>
        or std::same_as<std::remove_cv_t   <Type>, std::uintptr_t>
        or std::same_as<std::remove_cv_t   <Type>, std::intptr_t >
        or std::same_as<std::remove_cvref_t<Type>, va_t          >;

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
        else if constexpr ( std::same_as<std::remove_cvref_t<Addr>, va_t> )
            return static_cast<uptr>( base.get() );
        else
            return static_cast<uptr>( base );
    }
}


// IMPLEMENTATIONS -----------------------------------------------------------
template< typename Type, typename Tag >
class stx::details::strong_type
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

        friend constexpr strong_type operator+( strong_type lhs, Type rhs ) noexcept {
            lhs.value += rhs;
            return lhs;
        }

        friend constexpr strong_type operator-( strong_type lhs, Type rhs ) noexcept {
            lhs.value -= rhs;
            return lhs;
        }

        friend constexpr Type operator-( strong_type lhs, strong_type rhs ) noexcept {
            return lhs.value - rhs.value;
        }

        friend constexpr auto
        operator<=>(const strong_type&, const strong_type&) = default;

    private:
        Type value{};
};
