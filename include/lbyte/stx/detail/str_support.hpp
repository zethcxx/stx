#pragma once

#include <concepts>
#include <string_view>
#include <tuple>
#include <type_traits>

#if __has_include(<format>)
    #include <format>
#endif

// std::formatter + tuple protocol for ct::str_type.

namespace std
{
    template<::lbyte::stx::ct::fixed_string Str, typename CharT, typename... Flags>
    struct tuple_size<::lbyte::stx::ct::str_type<Str, CharT, Flags...>>
        : std::integral_constant<size_t, 2> {};

    template<size_t I, ::lbyte::stx::ct::fixed_string Str, typename CharT, typename... Flags>
        requires (I < 2)
    struct tuple_element<I, ::lbyte::stx::ct::str_type<Str, CharT, Flags...>> {
        using str = ::lbyte::stx::ct::str_type<Str, CharT, Flags...>;
        using type = std::conditional_t<I == 0, const typename str::char_type*, size_t>;
    };

#if __has_include(<format>)
    template<::lbyte::stx::ct::fixed_string Str, typename CharT, typename... Flags>
        requires std::same_as<typename ::lbyte::stx::ct::str_type<Str, CharT, Flags...>::char_type, char>
    struct formatter<::lbyte::stx::ct::str_type<Str, CharT, Flags...>>
        : std::formatter<std::string_view>
    {
        auto format(const ::lbyte::stx::ct::str_type<Str, CharT, Flags...>& s, auto& ctx) const {
            return std::formatter<std::string_view>::format(
                std::string_view{ s.data(), s.size() }, ctx);
        }
    };
#endif
}
