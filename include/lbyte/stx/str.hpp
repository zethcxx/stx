#pragma once
#include "./core.hpp"
#include <array>
#include <string>
#include <string_view>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-string-literal-operator-template"
#endif

namespace lbyte::stx::details
{
    template<typename CharT, size_t N>
    struct basic_sl
    {
        std::array<CharT, N> data_{};

        consteval basic_sl() noexcept = default;

        consteval basic_sl( const CharT (&s)[N] ) noexcept {
            for ( size_t i = 0; i < N; ++i )
                data_[i] = s[i];
        }

        consteval basic_sl( std::array<CharT, N> arr ) noexcept
            : data_( arr )
        {}

        // --- decaimiento natural (como un string literal crudo) --------------

        [[nodiscard]]
        constexpr operator const CharT*() const noexcept {
            return data_.data();
        }

        // --- acceso explícito -------------------------------------------------

        [[nodiscard]]
        constexpr std::basic_string_view<CharT> sv() const noexcept {
            size_t len = 0;
            while ( len < N && data_[len] != CharT{} )
                ++len;
            return { data_.data(), len };
        }

        [[nodiscard]]
        constexpr std::basic_string<CharT> str() const {
            return std::basic_string<CharT>{ sv() };
        }

        // --- transformaciones -------------------------------------------------

        [[nodiscard]]
        consteval auto trim() const noexcept -> basic_sl {
            basic_sl r{};

            size_t src = 0;
            while ( src < N && data_[src] == CharT{'\n'} )
                ++src;

            size_t dst = 0;
            while ( src < N && data_[src] != CharT{} )
                r.data_[dst++] = data_[src++];

            while ( dst > 0 && r.data_[dst - 1] == CharT{'\n'} )
                r.data_[--dst] = CharT{};

            return r;
        }

        [[nodiscard]]
        consteval auto unindent() const noexcept -> basic_sl {
            size_t indent     = 0;
            size_t line_start = 0;
            bool   searching  = true;

            for ( size_t i = 0; i < N && searching && data_[i] != CharT{}; ++i ) {
                auto c = data_[i];
                if ( c == CharT{'\n'} ) {
                    line_start = i + 1;
                } else if ( c == CharT{' '} || c == CharT{'\t'} ) {
                } else {
                    indent    = i - line_start;
                    searching = false;
                }
            }

            if ( searching || indent == 0 )
                return *this;

            basic_sl r{};
            size_t dst = 0;
            size_t col = 0;

            for ( size_t i = 0; i < N && data_[i] != CharT{}; ++i ) {
                auto c = data_[i];
                if ( c == CharT{'\n'} ) {
                    r.data_[dst++] = c;
                    col = 0;
                } else if ( col < indent && (c == CharT{' '} || c == CharT{'\t'}) ) {
                    ++col;
                } else {
                    r.data_[dst++] = c;
                    ++col;
                }
            }

            return r;
        }
    };
}

namespace lbyte::stx::literals
{
    template<typename CharT, CharT... Cs>
    [[nodiscard]] consteval auto operator""_sl() noexcept
        -> details::basic_sl<CharT, sizeof...(Cs) + 1>
    {
        return { std::array<CharT, sizeof...(Cs) + 1>{ Cs..., CharT{} } };
    }
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif
