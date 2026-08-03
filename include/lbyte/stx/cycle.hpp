#pragma once

#include "core.hpp"

namespace lbyte::stx
{
    namespace details
    {
        struct cycle_sentinel {};

        template<typename It, typename End>
        struct cycle_iter
        {
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

            [[nodiscard]] constexpr bool operator==( cycle_sentinel ) const noexcept
            {
                return passes_left_ == 0;
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
    // `cycle(r)`   — infinite repetition; end() is never reached, break/compose to stop.
    // `cycle(r, n)` — exactly `n` passes over the underlying range.
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
