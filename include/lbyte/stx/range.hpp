#pragma once

#include "core.hpp"

#include <iterator>
#include <ranges>

namespace lbyte::stx
{
    namespace details
    {
        template<typename Type>
        concept rangeable
            =  std::integral<Type>
            or std::is_enum_v<Type>
            or requires { typename Type::value_type; }
            or requires( Type t ) { requires std::integral<std::remove_cvref_t<decltype(t.get())>>; };

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

    // FACTORY FUNCTIONS ---------------------------------------------------------
    // Direction is inferred: 1-arg/2-arg from to>=from, 3-arg from step sign.
    //
    // Deduction overloads (preferred): infer Type from arguments.
    //    range(5)              → Type = int
    //    range(off_s{5})       → Type = off_s
    //    range(1, 10)          → Type = int (same-type args)
    //    range(1, 10, -1)      → Type = int
    //
    // Explicit overloads: require explicit template argument.
    //    range<usize>(off_s_var, rva_var)  → for mixed/non-common types

    // ── Deduction overloads ──────────────────────────────────────────────────

    template<details::rangeable T> [[nodiscard]]
    constexpr auto range( T _to ) noexcept
    {
        using Type = T;
        using ValueT = details::base_type_t<Type>;
        ValueT from{};
        ValueT to = details::unwrap( Type{ _to } );
        auto d = (to >= from) ? details::dir::fwd : details::dir::bwd;
        return details::range_view<Type>{ from, to, ValueT{ 1 }, d, range_mode::Exclusive };
    }

    template<details::rangeable T1, details::rangeable T2>
        requires std::same_as<T1, T2>
    [[nodiscard]] constexpr auto range( T1 _from, T2 _to ) noexcept
    {
        using Type = T1;
        using ValueT = details::base_type_t<Type>;
        ValueT from = details::unwrap( Type{ _from } );
        ValueT to   = details::unwrap( Type{ _to   } );
        auto d = (to >= from) ? details::dir::fwd : details::dir::bwd;
        return details::range_view<Type>{ from, to, ValueT{ 1 }, d, range_mode::Exclusive };
    }

    template<details::rangeable T1, details::rangeable T2>
        requires std::same_as<T1, T2>
    [[nodiscard]] constexpr auto range( T1 _from, T2 _to, auto _step ) noexcept
    {
        using Type = T1;
        using ValueT = details::base_type_t<Type>;
        using SignedT = std::make_signed_t<ValueT>;
        ValueT from = details::unwrap( Type{ _from } );
        ValueT to   = details::unwrap( Type{ _to   } );
        SignedT step = static_cast<SignedT>( _step );
        auto d = (step >= 0) ? details::dir::fwd : details::dir::bwd;
        using UnsignedT = std::make_unsigned_t<SignedT>;
        ValueT mag = static_cast<ValueT>( step >= 0 ? UnsignedT(step) : -UnsignedT(step) );
        return details::range_view<Type>{ from, to, mag, d, range_mode::Exclusive };
    }

    template<details::rangeable T> [[nodiscard]]
    constexpr auto irange( T _to ) noexcept
    {
        using Type = T;
        using ValueT = details::base_type_t<Type>;
        ValueT from{};
        ValueT to = details::unwrap( Type{ _to } );
        auto d = (to >= from) ? details::dir::fwd : details::dir::bwd;
        return details::range_view<Type>{ from, to, ValueT{ 1 }, d, range_mode::Inclusive };
    }

    template<details::rangeable T1, details::rangeable T2>
        requires std::same_as<T1, T2>
    [[nodiscard]] constexpr auto irange( T1 _from, T2 _to ) noexcept
    {
        using Type = T1;
        using ValueT = details::base_type_t<Type>;
        ValueT from = details::unwrap( Type{ _from } );
        ValueT to   = details::unwrap( Type{ _to   } );
        auto d = (to >= from) ? details::dir::fwd : details::dir::bwd;
        return details::range_view<Type>{ from, to, ValueT{ 1 }, d, range_mode::Inclusive };
    }

    template<details::rangeable T1, details::rangeable T2>
        requires std::same_as<T1, T2>
    [[nodiscard]] constexpr auto irange( T1 _from, T2 _to, auto _step ) noexcept
    {
        using Type = T1;
        using ValueT = details::base_type_t<Type>;
        using SignedT = std::make_signed_t<ValueT>;
        ValueT from = details::unwrap( Type{ _from } );
        ValueT to   = details::unwrap( Type{ _to   } );
        SignedT step = static_cast<SignedT>( _step );
        auto d = (step >= 0) ? details::dir::fwd : details::dir::bwd;
        using UnsignedT = std::make_unsigned_t<SignedT>;
        ValueT mag = static_cast<ValueT>( step >= 0 ? UnsignedT(step) : -UnsignedT(step) );
        return details::range_view<Type>{ from, to, mag, d, range_mode::Inclusive };
    }


}

// DETAILS IMPLEMENTATIONS ---------------------------------------------------
template<lbyte::stx::details::rangeable Type>
struct lbyte::stx::details::range_iter
{
    using ValueT = base_type_t<Type>;

    using difference_type = ::std::ptrdiff_t;
    using value_type      = Type;
    using reference       = Type;
    using iterator_concept  = ::std::input_iterator_tag;
    using iterator_category = ::std::input_iterator_tag;

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

    [[nodiscard]] constexpr range_iter operator++( int ) noexcept
    {
        auto copy = *this;
        ++*this;
        return copy;
    }

    [[nodiscard]] friend constexpr bool operator==( range_iter const& i, range_sentinel ) noexcept
    {
        return i.remaining == 0;
    }

    [[nodiscard]] friend constexpr bool operator==( range_sentinel, range_iter const& i ) noexcept
    {
        return i.remaining == 0;
    }

    [[nodiscard]] friend constexpr bool operator!=( range_iter const& i, range_sentinel ) noexcept
    {
        return i.remaining != 0;
    }

    [[nodiscard]] friend constexpr bool operator!=( range_sentinel, range_iter const& i ) noexcept
    {
        return i.remaining != 0;
    }
};

// RANGE VIEW -----------------------------------------------------------------
template<lbyte::stx::details::rangeable T>
struct lbyte::stx::details::range_view
{
    using Type   = T;
    using ValueT = details::base_type_t<T>;
    using iter_t = details::range_iter<T> ;

    ValueT from;
    ValueT to  ;
    ValueT step;

    dir       dir_ ;
    range_mode mode;

    [[nodiscard]] constexpr ::lbyte::stx::usize count() const noexcept
    {
        if ( step == 0 )
            return 0;

        if ( dir_ == dir::fwd )
        {
            if ( from > to )
                return 0;

            auto dist = static_cast<::lbyte::stx::usize>( to - from );
            auto step_u = static_cast<::lbyte::stx::usize>( step );

            if ( mode == range_mode::Exclusive )
                return dist == 0 ? 0 : (dist - 1) / step_u + 1;
            else
                return dist / step_u + 1;
        }
        else
        {
            if ( from < to )
                return 0;

            auto dist = static_cast<::lbyte::stx::usize>( from - to );
            auto step_u = static_cast<::lbyte::stx::usize>( step );

            if ( mode == range_mode::Exclusive )
                return dist == 0 ? 0 : (dist - 1) / step_u + 1;
            else
                return dist / step_u + 1;
        }
    }

    constexpr auto begin() const noexcept
    {
        return iter_t{ from, step, count(), dir_ };
    }

    constexpr auto end() const noexcept {
        return details::range_sentinel{};
    }

};

// std::ranges conformance ---------------------------------------------------
// `range_view` is a `view`: it owns only its scalar state and yields computed
// values, so it is safe to pass as a prvalue to range adaptors
// (`std::views::zip`, ...). It is intentionally NOT a `borrowed_range` — the
// view owns its bounds/step state, so an rvalue must not outlive itself.

template<lbyte::stx::details::rangeable T>
inline constexpr bool std::ranges::enable_view<lbyte::stx::details::range_view<T>> = true;
