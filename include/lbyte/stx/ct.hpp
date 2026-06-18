#pragma once
#include "../stx/core.hpp"
#include "../stx/endian.hpp"
#include <array>
#include <string>
#include <string_view>

namespace lbyte::stx::ct
{
    template<size_t N>
    struct fixed_string {
        char data[N]{};

        constexpr fixed_string() noexcept = default;
        constexpr fixed_string(const char (&str)[N]) noexcept {
            for (size_t i = 0; i < N; ++i)
                data[i] = str[i];
        }

        [[nodiscard]] constexpr size_t size() const noexcept { return N - 1; }
        [[nodiscard]] constexpr const char& operator[](size_t i) const noexcept { return data[i]; }
        [[nodiscard]] constexpr char& operator[](size_t i) noexcept { return data[i]; }
        [[nodiscard]] constexpr bool operator==(const fixed_string&) const = default;
    };

    // ── fmt ──────────────────────────────────────────────────────────────────────
    // Compile-time string formatting flags for str_type / str<>
    enum class fmt : unsigned char {
        none     = 0,
        strip    = 1 << 0,
        unindent = 1 << 1,
    };

    [[nodiscard]] constexpr fmt operator|(fmt a, fmt b) noexcept {
        return static_cast<fmt>(static_cast<unsigned char>(a) | static_cast<unsigned char>(b));
    }
    [[nodiscard]] constexpr fmt operator&(fmt a, fmt b) noexcept {
        return static_cast<fmt>(static_cast<unsigned char>(a) & static_cast<unsigned char>(b));
    }
    [[nodiscard]] constexpr fmt operator^(fmt a, fmt b) noexcept {
        return static_cast<fmt>(static_cast<unsigned char>(a) ^ static_cast<unsigned char>(b));
    }
    [[nodiscard]] constexpr fmt operator~(fmt a) noexcept {
        return static_cast<fmt>(~static_cast<unsigned char>(a));
    }

    // ── byte_block ──────────────────────────────────────────────────────────────
    template<size_t N>
    struct byte_block {
        u8 _[N];
        [[nodiscard]] constexpr const u8* data() const noexcept { return _; }
        [[nodiscard]] constexpr usize size() const noexcept { return N; }
    };

    // ── endian ───────────────────────────────────────────────────────────────────
    enum class endian : unsigned char {
        little = 0,
        big    = 1,
    };

    // ── details (internal helpers) ───────────────────────────────────────────────
    namespace details
    {
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

        using namespace ::lbyte::stx;

        template<endian O, fixed_string Str>
            requires (Str.size() > 0 && Str.size() <= 8)
        consteval auto make_istr() noexcept
        {
            constexpr auto N = Str.size();

            if constexpr ( N <= 1 )
            {
                u8 v{};
                for ( size_t i = 0; i < N; ++i )
                    v |= static_cast<u8>( static_cast<unsigned char>( Str.data[i] ) )
                        << ( i * 8 );
                return v;
            }
            else
            {
                using T = std::conditional_t<N == 2, u16,
                          std::conditional_t<N <= 4, u32, u64>>;
                T v{};
                for ( size_t i = 0; i < N; ++i )
                {
                    auto s = static_cast<size_t>( O == endian::little ? i : ( N - 1 - i ) );
                    v |= static_cast<T>( static_cast<unsigned char>( Str.data[i] ) )
                        << ( s * 8 );
                }
                return v;
            }
        }

        template<fixed_string Str>
            requires (Str.size() > 0)
        consteval auto make_vstr() noexcept
        {
            constexpr auto N = Str.size();
            ::lbyte::stx::ct::byte_block<N> blk{};
            for ( size_t i = 0; i < N; ++i )
                blk._[i] = static_cast<u8>( static_cast<unsigned char>( Str.data[i] ) );
            return blk;
        }
    }

    // ── str / str_type ────────────────────────────────────────────────────────────
    template<fixed_string Str, fmt... Flags>
    struct str_type {
    private:
        static consteval auto build() noexcept {
            constexpr size_t N = Str.size() + 1;
            std::array<char, N> data{};
            for (size_t i = 0; i < N; ++i)
                data[i] = Str.data[i];
            if constexpr ( ((Flags == fmt::unindent) || ...) )
                data = details::unindent_arr(data);
            if constexpr ( ((Flags == fmt::strip) || ...) )
                data = details::strip_arr(data);
            return data;
        }

    public:
        static constexpr std::array<char, Str.size() + 1> value = build();
        using char_type = char;
        using value_type = const char_type*;
        using view_type  = std::basic_string_view<char_type>;

        [[nodiscard]] constexpr const char_type* data() const noexcept { return value.data(); }
        [[nodiscard]] constexpr size_t size() const noexcept {
            size_t n = 0;
            while (n < value.size() && value[n]) ++n;
            return n;
        }

        [[nodiscard]] constexpr operator const char_type*() const noexcept {
            return value.data();
        }

        [[nodiscard]] constexpr operator view_type() const noexcept {
            return {value.data(), size()};
        }

        [[nodiscard]] constexpr std::basic_string<char_type> str() const {
            return std::basic_string<char_type>{ value.data() };
        }
    };

    template<fixed_string Str, fmt... Flags>
    constexpr str_type<Str, Flags...> str{};

    // ── istr ──────────────────────────────────────────────────────────────────────
    template<fixed_string Str, endian Order = endian::little>
    constexpr auto istr = details::make_istr<Order, Str>();

    // ── vstr ──────────────────────────────────────────────────────────────────────
    template<fixed_string Str>
    constexpr auto vstr = details::make_vstr<Str>();
}

