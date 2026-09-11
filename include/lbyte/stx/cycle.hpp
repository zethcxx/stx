#pragma once

#include "core.hpp"

#include <iterator>
#include <ranges>

namespace lbyte::stx
{
    namespace details
    {
        struct cycle_sentinel {};

        template<typename It, typename End>
        struct cycle_iter
        {
            using difference_type = ::std::iter_difference_t<It>;
            using value_type      = ::std::iter_value_t<It>;
            using reference       = ::std::iter_reference_t<It>;
            using iterator_concept  = ::std::input_iterator_tag;
            using iterator_category = ::std::input_iterator_tag;

            It    first_      ;
            It    cur_        ;
            End   end_        ;
            usize passes_left_;

            static constexpr usize max_pass() noexcept { return usize( -1 ); }

            [[nodiscard]] constexpr decltype(auto) operator*() const noexcept
            {
                return *cur_;
            }

            constexpr cycle_iter& operator++() noexcept
            {
                ++cur_;

                if ( cur_ == end_ )
                {
                    if ( passes_left_ == max_pass() )
                    {
                        cur_ = first_;
                    }
                    else if ( passes_left_ > 1 )
                    {
                        --passes_left_;
                        cur_ = first_;
                    }
                    else
                    {
                        passes_left_ = 0;
                    }
                }

                return *this;
            }

            [[nodiscard]] constexpr cycle_iter operator++( int ) noexcept
            {
                auto copy = *this;
                ++*this;
                return copy;
            }

            [[nodiscard]] friend constexpr bool operator==( cycle_iter const& i, cycle_sentinel ) noexcept
            {
                return i.passes_left_ == 0;
            }

            [[nodiscard]] friend constexpr bool operator==( cycle_sentinel, cycle_iter const& i ) noexcept
            {
                return i.passes_left_ == 0;
            }

            [[nodiscard]] friend constexpr bool operator!=( cycle_iter const& i, cycle_sentinel ) noexcept
            {
                return i.passes_left_ != 0;
            }

            [[nodiscard]] friend constexpr bool operator!=( cycle_sentinel, cycle_iter const& i ) noexcept
            {
                return i.passes_left_ != 0;
            }
        };

        template<typename It, typename End>
        struct cycle_view
        {
            using iter_t = cycle_iter<It, End>;

            It    first_      ;
            End   end_        ;
            usize passes_left_;

            [[nodiscard]] constexpr iter_t begin() const noexcept
            {
                if ( first_ == end_ or passes_left_ == 0 )
                    return { first_, first_, end_, 0 };

                return { first_, first_, end_, passes_left_ };
            }

            [[nodiscard]] constexpr cycle_sentinel end() const noexcept
            {
                return {};
            }
        };
    }

    template<typename R>
    concept cycleable
        = requires ( R&& r )
        {
            r.begin();
            r.end();
        };

    // FACTORIES ---------------------------------------------------------------
    // `cycle(r)`   - infinite repetition; end() is never reached, break/compose to stop.
    // `cycle(r, n)` - exactly `n` passes over the underlying range.
    //
    // Iterators are copied out of `r`; for value-based ranges (stx::range) the
    // view stays valid, for container iterators the caller must keep `r` alive
    // (same lifetime contract as standard views).

    template<cycleable R> [[nodiscard]]
    constexpr auto cycle( R&& r ) noexcept
    {
        auto first = r.begin();
        auto end   = r.end();
        using iter_t = decltype( first );
        return details::cycle_view{ first, end, details::cycle_iter<iter_t, decltype( end )>::max_pass() };
    }

    template<cycleable R> [[nodiscard]]
    constexpr auto cycle( R&& r, usize passes ) noexcept
    {
        auto first = r.begin();
        auto end   = r.end();
        return details::cycle_view{ first, end, passes };
    }
}

// std::ranges conformance ---------------------------------------------------
// `cycle_view` is a `view`: it copies the underlying iterators at construction
// and owns no elements, so it is safe to pass as a prvalue to range adaptors
// (`std::views::zip`, ...). It is intentionally NOT a `borrowed_range` - an
// rvalue view is only valid while its copied iterators are (same lifetime
// contract as standard views).

template<typename It, typename End>
inline constexpr bool std::ranges::enable_view<::lbyte::stx::details::cycle_view<It, End>> = true;
