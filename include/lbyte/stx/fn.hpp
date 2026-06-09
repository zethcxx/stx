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
        fn_t fn = nullptr; \
        \
        template<address_like Addr> \
        inline constexpr caller_t(Addr addr) noexcept \
            : fn(reinterpret_cast<fn_t>(normalize_addr(addr))) \
        {} \
        \
        inline constexpr caller_t(std::nullptr_t) noexcept \
            : fn(nullptr) \
        {} \
        \
        inline constexpr operator fn_t      ()             const noexcept { return fn; }; \
        inline constexpr Ret      operator()(Args... args) const \
            noexcept(std::is_nothrow_invocable_v<fn_t, Args...>) \
            { return fn(args...); } \
        [[nodiscard]] constexpr explicit operator bool() const noexcept { return fn != nullptr; } \
    };


    LBYTE_SPEC_CALLER()

    #if defined(_MSC_VER)
        #if defined(__i386__) || defined(_M_IX86)
            LBYTE_SPEC_CALLER(__stdcall)
            LBYTE_SPEC_CALLER(__fastcall)
            LBYTE_SPEC_CALLER(__thiscall)
        #endif
        #if !defined(_M_CEE)
            LBYTE_SPEC_CALLER(__vectorcall)
        #endif
    #elif defined(__vectorcall__)
        LBYTE_SPEC_CALLER(__vectorcall)
    #endif

    #undef LBYTE_SPEC_CALLER

    template<class Sig> [[nodiscard]]
    inline constexpr auto caller( address_like auto addr ) noexcept
    {
        return caller_t<Sig>( addr );
    }
}

