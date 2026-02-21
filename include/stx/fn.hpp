#pragma once
#include "core.hpp"

namespace stx
{
    template<class Sig>
    struct caller_t;

    template<class Ret, class... Args>
    struct caller_t<Ret(Args...)>
    {
        using fn_t = Ret (*)(Args...);
        fn_t fn;

        inline constexpr Ret operator()(Args... args) const noexcept
        {
            return fn( args... );
        }

        [[nodiscard]] constexpr explicit operator bool() const noexcept {
            return fn != nullptr;
        }
    };

    template<class Sig> [[nodiscard]]
    inline constexpr auto caller( address_like auto addr ) noexcept
    {
        using fn_t = typename caller_t<Sig>::fn_t;
        return caller_t<Sig>{
            reinterpret_cast<fn_t>(normalize_addr(addr))
        };
    }
}
