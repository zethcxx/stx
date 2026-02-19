#pragma once

#include "./core.hpp"

namespace stx
{
    namespace details
    {
        template<class Type>
        concept range_integral
            =       std::is_integral_v<Type>
            and not std::is_same_v    <Type, bool>;

        struct range_sentinel {};

        template<std::integral Type>
        struct range_iter;

        template<std::integral Type>
        struct range_view;
    }

    // RANGE IMPLEMENTATION ------------------------------------------------------
    template<details::range_integral Type> [[nodiscard]]
    constexpr auto range( Type to ) noexcept
    {
        using Flag = typename details::range_iter<Type>::Flag;

        return details::range_view<Type>{
            Type{ 0 },
            to       ,
            Type{ 1 },
            Flag::Forward,
            Flag::NonInclusive
        };
    }

    template<std::integral Type> [[nodiscard]]
    constexpr auto range( Type from, Type to, Type step = 1 ) noexcept
    {
        if ( step == 0 )
            step = 1;

        using Flag = typename details::range_iter<Type>::Flag;

        return details::range_view<Type> {
            from,
            to  ,
            step,
            ( from <= to ) ? Flag::Forward : Flag::Backward,
            Flag::NonInclusive
        };
    }

    template<std::integral Type> [[nodiscard]]
    constexpr auto irange( Type from, Type to, Type step = 1 ) noexcept
    {
        if ( step == 0 )
            step = 1;

        using Flag = typename details::range_iter<Type>::Flag;

        return details::range_view<Type>{
            from,
            to  ,
            step,
            ( from <= to ) ? Flag::Forward : Flag::Backward,
            Flag::Inclusive
        };
    }
}

// DETAILS IMPLEMENTATIONS ---------------------------------------------------
template<std::integral Type>
struct stx::details::range_iter
{
    enum class Flag : u8
    {
        Forward     ,
        Backward    ,
        Inclusive   ,
        NonInclusive,
    };

    Type cur ;
    Type end ;
    Type step;

    Flag direction;
    Flag inclusive;

    constexpr Type operator*() const noexcept {
        return cur;
    }

    constexpr range_iter& operator++() noexcept
    {
        if ( direction == Flag::Forward )
            if ( cur > end || end - cur < step )
                cur = end;
            else
                cur += step;

        else // Backward
            if ( cur < end || cur - end < step )
                cur = end;
            else
                cur -= step;

        return *this;
    }

    constexpr bool operator!=( range_sentinel ) const noexcept
    {
        if ( inclusive == Flag::Inclusive )
            return ( direction == Flag::Forward )
                ? cur <= end
                : cur >= end;

        else // NonInclusive
            return cur != end;
    }
};

template<std::integral Type>
struct stx::details::range_view
{
    using iter_t = range_iter<Type>;
    using Flag   = typename iter_t::Flag;

    Type from;
    Type to  ;
    Type step;

    Flag direction;
    Flag inclusive;

    constexpr auto begin() const noexcept
    {
        return iter_t {
            from     ,
            to       ,
            step     ,
            direction,
            inclusive
        };
    }

    constexpr auto end() const noexcept {
        return range_sentinel{};
    }
};
