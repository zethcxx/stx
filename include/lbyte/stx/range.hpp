#pragma once

#include "./core.hpp"
#include <cassert>

namespace lbyte::stx
{
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
        struct base_type<strong_type<T, Tag>> { using type = T; };

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
    enum class range_dir: u8 {
        Forward,
        Backward
    };

    enum class range_mode : u8
    {
        Inclusive,
        Exclusive,
    };

    // FACTORY FUNCTIONS ----------------------------------------------------------
    template<details::rangeable Type> [[nodiscard]]
    constexpr auto range( Type _to ) noexcept {
        return range( _to, range_dir::Forward, range_mode::Exclusive );
    }

    template<details::rangeable Type> [[nodiscard]]
    constexpr auto range(
        Type _from,
        Type _to
    ) noexcept {
        return range( _from, _to, range_dir::Forward, range_mode::Exclusive );
    }

    template<details::rangeable Type> [[nodiscard]]
    constexpr auto range(
        Type _to,
        range_dir  dir,
        range_mode flag = range_mode::Exclusive
    ) noexcept {
        using ValueT = details::base_type_t<Type>;
        const ValueT to { details::unwrap( _to )};

        if (dir == range_dir::Backward) {
            return details::range_view<Type> {
                to,
                ValueT{ 0 },
                ValueT{ 1 },
                dir,
                flag
            };
        }

        return details::range_view<Type> {
            ValueT{ 0 },
            to         ,
            ValueT{ 1 },
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

    // --- ORIGINAL irange OVERLOADS (deduced Type from Type args) ---------------

    template<details::rangeable Type> [[nodiscard]]
    constexpr auto irange( Type from, Type to, details::base_type_t<Type> step, range_dir dir ) noexcept
    {
        return range( from, to, step, dir, range_mode::Inclusive );
    }

    template<details::rangeable Type> [[nodiscard]]
    constexpr auto irange( Type to ) noexcept
    {
        return range( to, range_dir::Forward, range_mode::Inclusive );
    }

    template<details::rangeable Type> [[nodiscard]]
    constexpr auto irange( Type to, range_dir dir ) noexcept
    {
        return range( to, dir, range_mode::Inclusive );
    }

    template<details::rangeable Type> [[nodiscard]]
    constexpr auto irange( Type from, Type to, range_dir dir ) noexcept
    {
        return range( from, to, details::base_type_t<Type>{ 1 }, dir, range_mode::Inclusive );
    }

    // --- CONVENIENCE OVERLOADS (explicit Type, auto-converting args) ------------
    // Enable ergonomic range<off_s>(0, count, 16) instead of
    // range(off_s{0}, off_s{count}, off_s{16}).  Only enabled when Type is a
    // strong type or enum (i.e. Type != base_type_t<Type>) so there is no
    // ambiguity with plain integral ranges.  The original Type-param overloads
    // are more specialised and win when both match.

    template<details::rangeable Type> [[nodiscard]]
    constexpr auto range( auto _to ) noexcept
        requires (not std::same_as<details::base_type_t<Type>, Type>)
    {
        return range( Type{ _to } );
    }

    template<details::rangeable Type> [[nodiscard]]
    constexpr auto range( auto _from, auto _to ) noexcept
        requires (not std::same_as<details::base_type_t<Type>, Type>)
    {
        return range( Type{ _from }, Type{ _to } );
    }

    template<details::rangeable Type> [[nodiscard]]
    constexpr auto range( auto _from, auto _to, auto step ) noexcept
        requires (not std::same_as<details::base_type_t<Type>, Type>)
    {
        return range(
            Type{ _from }, Type{ _to },
            static_cast<details::base_type_t<Type>>( step ),
            range_dir::Forward, range_mode::Exclusive
        );
    }

    template<details::rangeable Type> [[nodiscard]]
    constexpr auto range(
        auto      _from,
        auto      _to  ,
        auto      step ,
        range_dir dir  ,
        range_mode flag = range_mode::Exclusive
    ) noexcept
        requires (not std::same_as<details::base_type_t<Type>, Type>)
    {
        return range( Type{ _from }, Type{ _to },
            static_cast<details::base_type_t<Type>>( step ), dir, flag );
    }

    template<details::rangeable Type> [[nodiscard]]
    constexpr auto range(
        auto      from,
        auto      to  ,
        range_dir dir ,
        range_mode flag = range_mode::Exclusive
    ) noexcept
        requires (not std::same_as<details::base_type_t<Type>, Type>)
    {
        return range( Type{ from }, Type{ to },
            details::base_type_t<Type>{ 1 }, dir, flag );
    }

    template<details::rangeable Type> [[nodiscard]]
    constexpr auto range(
        auto      _to,
        range_dir dir,
        range_mode flag = range_mode::Exclusive
    ) noexcept
        requires (not std::same_as<details::base_type_t<Type>, Type>)
    {
        return range( Type{ _to }, dir, flag );
    }

    template<details::rangeable Type> [[nodiscard]]
    constexpr auto irange( auto to ) noexcept
        requires (not std::same_as<details::base_type_t<Type>, Type>)
    {
        return irange( Type{ to } );
    }

    template<details::rangeable Type> [[nodiscard]]
    constexpr auto irange( auto to, range_dir dir ) noexcept
        requires (not std::same_as<details::base_type_t<Type>, Type>)
    {
        return irange( Type{ to }, dir );
    }

    template<details::rangeable Type> [[nodiscard]]
    constexpr auto irange( auto from, auto to, range_dir dir ) noexcept
        requires (not std::same_as<details::base_type_t<Type>, Type>)
    {
        return irange( Type{ from }, Type{ to }, dir );
    }

    template<details::rangeable Type> [[nodiscard]]
    constexpr auto irange( auto from, auto to, auto step, range_dir dir ) noexcept
        requires (not std::same_as<details::base_type_t<Type>, Type>)
    {
        return irange( Type{ from }, Type{ to },
            static_cast<details::base_type_t<Type>>( step ), dir );
    }
}

// DETAILS IMPLEMENTATIONS ---------------------------------------------------
template<lbyte::stx::details::rangeable Type>
struct lbyte::stx::details::range_iter
{
    using ValueT = base_type_t<Type>;

    ValueT cur ;
    ValueT step;

    ::lbyte::stx::usize remaining;

    range_dir dir ;

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
            if ( dir == range_dir::Forward )
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
template<lbyte::stx::details::rangeable T>
struct lbyte::stx::details::range_view
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
        ::lbyte::stx::usize remaining = 0;

        assert( step != 0 && "range: step must be non-zero" );

        if ( dir == range_dir::Forward )
        {
            if ( from <= to )
            {
                auto dist = static_cast<::lbyte::stx::usize>( to - from );
                auto step_u = static_cast<::lbyte::stx::usize>( step );

                if ( mode == range_mode::Exclusive )
                    remaining = (dist + step_u - 1) / step_u;  // ceiling — include last partial step
                else // Inclusive
                    remaining = dist / step_u + 1;
            }
        }
        else // Backward
        {
            if ( from >= to )
            {
                auto dist = static_cast<::lbyte::stx::usize>( from - to );
                auto step_u = static_cast<::lbyte::stx::usize>( step );

                if ( mode == range_mode::Exclusive )
                    remaining = (dist + step_u - 1) / step_u;  // ceiling — same as forward exclusive
                else // Inclusive
                    remaining = dist / step_u + 1;
            }
        }

        auto it = iter_t { from, step, remaining, dir };

        return it;
    }

    constexpr auto end() const noexcept {
        return stx::details::range_sentinel{};
    }
};

