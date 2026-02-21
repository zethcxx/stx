#pragma once

#include "./core.hpp"
#include <cassert>

namespace stx
{
    namespace details
    {
        template<typename Type>
        concept rangeable
            =  std::integral<Type>
            or requires( Type t ) {
                typename Type::value_type;
                { t.get() } -> std::integral;
            };

        template<typename T>
        struct base_type { using type = T; };

        template<typename T, typename Tag>
        struct base_type<strong_type<T, Tag>> { using type = T; };

        template<typename T>
        using base_type_t = typename base_type<T>::type;

        struct range_sentinel {};

        template<rangeable T>
        struct range_iter;

        template<rangeable T>
        struct range_view;

        template<typename T>
        constexpr auto unwrap( T value ) noexcept
        {
            if constexpr ( std::integral<T> )
                return static_cast<base_type_t<T>>( value );
            else
                return value.get();
        }
    }

    enum class range_dir: u8 {
        Forward,
        Backward
    };

    enum class range_mode : u8
    {
        Inclusive,
        Exclusive,
    };

    // RANGE IMPLEMENTATION ------------------------------------------------------
    template<details::rangeable Type> [[nodiscard]]
    constexpr auto range(
        Type _to,
        range_dir  dir,
        range_mode flag = range_mode::Exclusive
    ) noexcept {
        using ValueT = details::base_type_t<Type>;
        const ValueT to { details::unwrap( _to )};

        return details::range_view<Type> {
            ValueT{ 0 },
            to         ,
            details::base_type_t<Type>{ 1 },
            dir,
            flag
        };
    }

    template<details::rangeable Type> [[nodiscard]]
    constexpr auto range(
        Type      _from,
        Type      _to  ,
        details::base_type_t<Type> step,
        range_dir dir  ,
        range_mode flag = range_mode::Exclusive
    ) noexcept {
        using ValueT = details::base_type_t<Type>;

        const ValueT from { details::unwrap( _from ) };
        const ValueT to   { details::unwrap( _to   ) };

        return details::range_view<Type> {
            from,
            to  ,
            step,
            dir,
            flag
        };
    }

    template<details::rangeable Type> [[nodiscard]]
    constexpr auto range(
        Type      from,
        Type      to  ,
        range_dir dir ,
        range_mode flag = range_mode::Exclusive
    ) noexcept {
        return range( from, to, details::base_type_t<Type>{ 1 }, dir, flag );
    }

    template<details::rangeable Type> [[nodiscard]]
    constexpr auto irange( Type from, Type to, details::base_type_t<Type> step, range_dir dir ) noexcept
    {
        return range( from, to, step, dir, range_mode::Inclusive );
    }

    template<details::rangeable Type> [[nodiscard]]
    constexpr auto irange( Type to, range_dir dir ) noexcept
    {
        return range( to, dir, range_mode::Inclusive );
    }
}

// DETAILS IMPLEMENTATIONS ---------------------------------------------------
template<stx::details::rangeable Type>
struct stx::details::range_iter
{
    using ValueT = base_type_t<Type>;

    ValueT cur ;
    ValueT end ;
    ValueT step;

    range_dir  dir ;
    range_mode mode;

    constexpr Type operator*() const noexcept
    {
        if constexpr ( std::integral<Type> )
            return cur;
        else
            return Type { cur };
    }

    constexpr range_iter& operator++() noexcept
    {
        if ( dir == range_dir::Forward )
            cur += step;
        else
            cur -= step;

        return *this;
    }

    constexpr bool operator==( range_sentinel ) const noexcept
    {
        if ( dir == range_dir::Forward )
        {
            if ( mode == range_mode::Inclusive )
                return cur > end;
            else
                return cur >= end;
        }
        else // Backward
        {
            if ( mode == range_mode::Inclusive )
                return cur < end;
            else
                return cur <= end;
        }
    }
};

template<stx::details::rangeable T>
struct stx::details::range_view
{
    using ValueT = stx::details::base_type_t<T>;
    using iter_t = stx::details::range_iter<T> ;

    ValueT from;
    ValueT to  ;
    ValueT step;

    range_dir  dir ;
    range_mode mode;

    constexpr auto begin() const noexcept
    {
        return iter_t {
            from,
            to  ,
            step,
            dir ,
            mode
        };
    }

    constexpr auto end() const noexcept {
        return stx::details::range_sentinel{};
    }
};
