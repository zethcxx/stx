#pragma once
#include "core.hpp"

namespace lbyte::stx
{
    template<class Sig>
    struct caller_t;

    #define LBYTE_SPEC_CALLER(CC) \
    template<class Ret, class... Args> \
    struct caller_t<Ret CC(Args...)> \
    { \
        using fn_t = Ret (CC *)(Args...); \
        fn_t fn; \
        inline constexpr operator fn_t      ()             const noexcept { return fn; }; \
        inline constexpr Ret      operator()(Args... args) const noexcept { return fn(args...); } \
        [[nodiscard]] constexpr explicit operator bool() const noexcept { return fn != nullptr; } \
    };


    LBYTE_SPEC_CALLER()

    #if defined(__i386__) || defined(_M_IX86)
        LBYTE_SPEC_CALLER(__stdcall)
        LBYTE_SPEC_CALLER(__fastcall)
        LBYTE_SPEC_CALLER(__thiscall)
    #endif

    #if defined(__vectorcall__) || defined(_MSC_EXTENSIONS)
        #if !defined(__x86_64__) && !defined(_M_X64)
            LBYTE_SPEC_CALLER(__vectorcall)
        #endif
    #endif

    #undef LBYTE_SPEC_CALLER

    template<class Sig> [[nodiscard]]
    inline constexpr auto caller( address_like auto addr ) noexcept
    {
        using fn_t = typename caller_t<Sig>::fn_t;
        return caller_t<Sig>{
            reinterpret_cast<fn_t>(normalize_addr(addr))
        };
    }
}
