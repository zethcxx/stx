#pragma once
#include "./core.hpp"
#include <array>
#include <string>

namespace lbyte::stx::lit
{
    namespace details
    {
        template<size_t N>
        struct fixed_string {
            char data[N]{};

            constexpr fixed_string(const char (&str)[N]) noexcept {
                for (size_t i = 0; i < N; ++i)
                    data[i] = str[i];
            }

            [[nodiscard]] constexpr size_t size() const noexcept { return N - 1; }
            [[nodiscard]] constexpr bool operator==(const fixed_string&) const = default;
        };

        template<size_t N>
        [[nodiscard]] consteval std::array<char, N> strip_arr(std::array<char, N> data) noexcept {
            size_t null_pos = 0;
            while (null_pos < N && data[null_pos] != '\0') ++null_pos;
            if (null_pos == 0) return data;

            size_t first_newline = 0;
            bool first_empty = true;
            for (; first_newline < null_pos; ++first_newline) {
                if (data[first_newline] == '\n') break;
                if (data[first_newline] != ' ' && data[first_newline] != '\t') {
                    first_empty = false;
                    break;
                }
            }
            if (!first_empty) first_newline = 0;
            else if (first_newline < null_pos) ++first_newline;

            size_t last_newline = 0;
            bool found_newline = false;
            for (size_t i = 0; i < null_pos; ++i)
                if (data[i] == '\n') { last_newline = i; found_newline = true; }

            bool last_empty = false;
            if (found_newline) {
                last_empty = true;
                for (size_t i = last_newline + 1; i < null_pos; ++i) {
                    if (data[i] != ' ' && data[i] != '\t') {
                        last_empty = false;
                        break;
                    }
                }
            }

            size_t end = last_empty ? last_newline : null_pos;

            std::array<char, N> result{};
            for (size_t i = first_newline; i < end; ++i)
                result[i - first_newline] = data[i];
            return result;
        }

        template<size_t N>
        [[nodiscard]] consteval std::array<char, N> unindent_arr(std::array<char, N> data) noexcept {
            size_t indent = 0, line_start = 0;
            bool searching = true;
            for (size_t i = 0; i < N && data[i] != '\0'; ++i) {
                auto c = data[i];
                if (searching) {
                    if (c == '\n') {
                        line_start = i + 1;
                    } else if (c != ' ' && c != '\t') {
                        indent = i - line_start;
                        break;
                    }
                }
            }

            std::array<char, N> result{};
            size_t dst = 0, col = 0;
            searching = true;
            for (size_t i = 0; i < N && data[i] != '\0'; ++i) {
                auto c = data[i];
                if (searching) {
                    if (c == '\n') {
                        result[dst++] = c;
                        col = 0;
                    } else if (c != ' ' && c != '\t') {
                        result[dst++] = c; ++col;
                        searching = false;
                    } else if (col < indent) {
                        ++col;
                    } else {
                        result[dst++] = c; ++col;
                    }
                } else {
                    if (c == '\n') {
                        result[dst++] = c;
                        col = 0;
                        searching = true;
                    } else {
                        result[dst++] = c; ++col;
                    }
                }
            }
            return result;
        }

        enum class flag : unsigned char {
            none     = 0,
            strip    = 1 << 0,
            unindent = 1 << 1,
        };

        [[nodiscard]] constexpr flag operator|(flag a, flag b) noexcept {
            return static_cast<flag>(static_cast<unsigned char>(a) | static_cast<unsigned char>(b));
        }

        [[nodiscard]] constexpr flag operator&(flag a, flag b) noexcept {
            return static_cast<flag>(static_cast<unsigned char>(a) & static_cast<unsigned char>(b));
        }

        [[nodiscard]] constexpr flag operator^(flag a, flag b) noexcept {
            return static_cast<flag>(static_cast<unsigned char>(a) ^ static_cast<unsigned char>(b));
        }

        [[nodiscard]] constexpr flag operator~(flag a) noexcept {
            return static_cast<flag>(~static_cast<unsigned char>(a));
        }
    }

    constexpr auto strip    = details::flag::strip;
    constexpr auto unindent = details::flag::unindent;

    template<details::fixed_string Str, details::flag... Flags>
    struct str_type {
    private:
        static consteval auto build() noexcept {
            constexpr size_t N = Str.size() + 1;
            std::array<char, N> data{};
            for (size_t i = 0; i < N; ++i)
                data[i] = Str.data[i];
            if constexpr ( ((Flags == details::flag::unindent) || ...) )
                data = details::unindent_arr(data);
            if constexpr ( ((Flags == details::flag::strip) || ...) )
                data = details::strip_arr(data);
            return data;
        }

    public:
        static constexpr std::array<char, Str.size() + 1> value = build();

        using char_type = char;

        [[nodiscard]] constexpr operator const char_type*() const noexcept {
            return value.data();
        }

        [[nodiscard]] constexpr std::basic_string<char_type> str() const {
            return std::basic_string<char_type>{ value.data() };
        }
    };

    template<details::fixed_string Str, details::flag... Flags>
    constexpr str_type<Str, Flags...> str{};
}

