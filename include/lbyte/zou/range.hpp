#pragma once

#include "../stx/core.hpp"
#include <cassert>

namespace lbyte::zou
{
    using namespace lbyte::stx;
    namespace details
    {
        template<typename Type>
        concept rangeable
            =  std::integral<Type>
            or std::is_enum_v<Type>
            or requires { typename Type::value_type; }
            or requires( Type t ) { { t.get() } -> std::integral; };

        template<typename T>
        struct base_type { using type = T; };

        template<typename T, typename Tag>
        struct base_type<::lbyte::stx::details::strong_type<T, Tag>> { using type = T; };

        template<typename T> requires std::is_enum_v<T>
        struct base_type<T> { using type = std::underlying_type_t<T>; };

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
            if constexpr ( std::integral<T> or std::is_enum_v<T> )
                return static_cast<base_type_t<T>>( value );
            else
                return value.get();
        }
    }

    // ENUMS ---------------------------------------------------------------------
    enum class range_mode : ::lbyte::stx::u8
    {
        Inclusive,
        Exclusive,
    };

    namespace details {
        enum class dir : ::lbyte::stx::u8 {
            fwd,
            bwd,
        };
    }

    // FACTORY FUNCTIONS (explicit Type only, auto-converting args) ---------------
    // Direction is inferred: 1-arg/2-arg from to>=from, 3-arg from step sign.

    template<details::rangeable Type> [[nodiscard]]
    constexpr auto range( auto _to ) noexcept
    {
        using ValueT = details::base_type_t<Type>;
        using SignedT = std::make_signed_t<ValueT>;
        ValueT from{};
        ValueT to = details::unwrap( Type{ _to } );
        auto d = (to >= from) ? details::dir::fwd : details::dir::bwd;
        return details::range_view<Type>{ from, to, ValueT{ 1 }, d, range_mode::Exclusive };
    }

    template<details::rangeable Type> [[nodiscard]]
    constexpr auto range( auto _from, auto _to ) noexcept
    {
        using ValueT = details::base_type_t<Type>;
        ValueT from = details::unwrap( Type{ _from } );
        ValueT to   = details::unwrap( Type{ _to   } );
        auto d = (to >= from) ? details::dir::fwd : details::dir::bwd;
        return details::range_view<Type>{ from, to, ValueT{ 1 }, d, range_mode::Exclusive };
    }

    template<details::rangeable Type> [[nodiscard]]
    constexpr auto range( auto _from, auto _to, auto _step ) noexcept
    {
        using ValueT = details::base_type_t<Type>;
        using SignedT = std::make_signed_t<ValueT>;
        ValueT from = details::unwrap( Type{ _from } );
        ValueT to   = details::unwrap( Type{ _to   } );
        SignedT step = static_cast<SignedT>( _step );
        auto d = (step >= 0) ? details::dir::fwd : details::dir::bwd;
        ValueT mag = static_cast<ValueT>( step >= 0 ? step : -step );
        return details::range_view<Type>{ from, to, mag, d, range_mode::Exclusive };
    }

    template<details::rangeable Type> [[nodiscard]]
    constexpr auto irange( auto _to ) noexcept
    {
        using ValueT = details::base_type_t<Type>;
        ValueT from{};
        ValueT to = details::unwrap( Type{ _to } );
        auto d = (to >= from) ? details::dir::fwd : details::dir::bwd;
        return details::range_view<Type>{ from, to, ValueT{ 1 }, d, range_mode::Inclusive };
    }

    template<details::rangeable Type> [[nodiscard]]
    constexpr auto irange( auto _from, auto _to ) noexcept
    {
        using ValueT = details::base_type_t<Type>;
        ValueT from = details::unwrap( Type{ _from } );
        ValueT to   = details::unwrap( Type{ _to   } );
        auto d = (to >= from) ? details::dir::fwd : details::dir::bwd;
        return details::range_view<Type>{ from, to, ValueT{ 1 }, d, range_mode::Inclusive };
    }

    template<details::rangeable Type> [[nodiscard]]
    constexpr auto irange( auto _from, auto _to, auto _step ) noexcept
    {
        using ValueT = details::base_type_t<Type>;
        using SignedT = std::make_signed_t<ValueT>;
        ValueT from = details::unwrap( Type{ _from } );
        ValueT to   = details::unwrap( Type{ _to   } );
        SignedT step = static_cast<SignedT>( _step );
        auto d = (step >= 0) ? details::dir::fwd : details::dir::bwd;
        ValueT mag = static_cast<ValueT>( step >= 0 ? step : -step );
        return details::range_view<Type>{ from, to, mag, d, range_mode::Inclusive };
    }
}

// DETAILS IMPLEMENTATIONS ---------------------------------------------------
template<lbyte::zou::details::rangeable Type>
struct lbyte::zou::details::range_iter
{
    using ValueT = base_type_t<Type>;

    ValueT cur ;
    ValueT step;

    ::lbyte::stx::usize remaining;

    dir dir_ ;

    constexpr Type operator*() const noexcept
    {
        if constexpr ( std::integral<Type> )
            return cur;
        else
            return Type { cur };
    }

    constexpr range_iter& operator++() noexcept
    {
        if ( remaining > 0 )
        {
            if ( dir_ == dir::fwd )
                cur += step;
            else
                cur -= step;

            --remaining;
        }

        return *this;
    }

    constexpr bool operator==( range_sentinel ) const noexcept
    {
        return remaining == 0;
    }
};

// RANGE VIEW -----------------------------------------------------------------
template<lbyte::zou::details::rangeable T>
struct lbyte::zou::details::range_view
{
    using ValueT = details::base_type_t<T>;
    using iter_t = details::range_iter<T> ;

    ValueT from;
    ValueT to  ;
    ValueT step;

    dir       dir_ ;
    range_mode mode;

    constexpr auto begin() const noexcept
    {
        ::lbyte::stx::usize remaining = 0;

        assert( step != 0 && "range: step must be non-zero" );

        if ( dir_ == dir::fwd )
        {
            if ( from <= to )
            {
                auto dist = static_cast<::lbyte::stx::usize>( to - from );
                auto step_u = static_cast<::lbyte::stx::usize>( step );

                if ( mode == range_mode::Exclusive )
                    remaining = (dist + step_u - 1) / step_u;
                else
                    remaining = dist / step_u + 1;
            }
        }
        else
        {
            if ( from >= to )
            {
                auto dist = static_cast<::lbyte::stx::usize>( from - to );
                auto step_u = static_cast<::lbyte::stx::usize>( step );

                if ( mode == range_mode::Exclusive )
                    remaining = (dist + step_u - 1) / step_u;
                else
                    remaining = dist / step_u + 1;
            }
        }

        auto it = iter_t { from, step, remaining, dir_ };

        return it;
    }

    constexpr auto end() const noexcept {
        return details::range_sentinel{};
    }
};
