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

        // --- decay to C string ------------------------------------------------

        [[nodiscard]]
        constexpr operator const CharT*() const & noexcept {
            return data_.data();
        }

        [[nodiscard]]
        constexpr operator const CharT*() const && noexcept = delete;

        // --- explicit access --------------------------------------------------

        [[nodiscard]]
        constexpr std::basic_string_view<CharT> sv() const & noexcept {
            size_t len = 0;
            while ( len < N && data_[len] != CharT{} )
                ++len;
            return { data_.data(), len };
        }

        [[nodiscard]]
        constexpr std::basic_string_view<CharT> sv() const && noexcept = delete;

        [[nodiscard]]
        constexpr const CharT* c_str() const & noexcept {
            return data_.data();
        }

        [[nodiscard]]
        constexpr const CharT* c_str() const && noexcept = delete;

        [[nodiscard]]
        constexpr std::basic_string<CharT> str() const {
            return std::basic_string<CharT>{ sv() };
        }

        // --- transformations --------------------------------------------------

    private:
        [[nodiscard]]
        consteval basic_sl<CharT, N> do_trim() const noexcept {
            size_t src = 0;
            while ( src < N && data_[src] == CharT{'\n'} )
                ++src;
            size_t end = src;
            while ( end < N && data_[end] != CharT{} )
                ++end;
            while ( end > src && data_[end - 1] == CharT{'\n'} )
                --end;
            std::array<CharT, N> result{};
            for ( size_t i = 0; i < end - src; ++i )
                result[i] = data_[src + i];
            return { result };
        }

        [[nodiscard]]
        consteval basic_sl<CharT, N> do_unindent() const noexcept {
            size_t indent = 0, line_start = 0;
            bool searching = true;
            for ( size_t i = 0; i < N && data_[i] != CharT{}; ++i ) {
                auto c = data_[i];
                if ( searching ) {
                    if ( c == CharT{'\n'} ) { line_start = i + 1; }
                    else if ( c != CharT{' '} && c != CharT{'\t'} ) {
                        indent = i - line_start;
                        break;
                    }
                }
            }
            std::array<CharT, N> result{};
            size_t dst = 0, col = 0;
            searching = true;
            for ( size_t i = 0; i < N && data_[i] != CharT{}; ++i ) {
                auto c = data_[i];
                if ( searching ) {
                    if ( c == CharT{'\n'} ) { result[dst++] = c; col = 0; }
                    else if ( c != CharT{' '} && c != CharT{'\t'} ) {
                        result[dst++] = c; ++col;
                        searching = false;
                    }
                    else if ( col < indent ) { ++col; }
                    else { result[dst++] = c; ++col; }
                } else {
                    if ( c == CharT{'\n'} ) { result[dst++] = c; col = 0; searching = true; }
                    else { result[dst++] = c; ++col; }
                }
            }
            return { result };
        }

    public:
        [[nodiscard]]
        constexpr auto trim() const noexcept {
            return do_trim();
        }

        [[nodiscard]]
        constexpr auto unindent() const noexcept {
            return do_unindent();
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
