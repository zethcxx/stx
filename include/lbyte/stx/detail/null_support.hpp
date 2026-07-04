#pragma once

#include <functional>

template<>
struct std::hash<::lbyte::stx::null_t> {
    constexpr std::size_t operator()(::lbyte::stx::null_t) const noexcept { return 0; }
};

#if __has_include(<format>)
    #include <format>

    template<>
    struct std::formatter<::lbyte::stx::null_t> {
        constexpr auto parse(auto& ctx) { return ctx.begin(); }
        auto format(::lbyte::stx::null_t, auto& ctx) const {
            return std::format_to(ctx.out(), "null");
        }
    };
#endif
