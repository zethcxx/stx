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

    // --- fmt ---------------------------------------------------------------------
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

    // --- byte_block --------------------------------------------------------------
    template<size_t N>
    struct byte_block {
        u8 _[N];
        [[nodiscard]] constexpr const u8* data() const noexcept { return _; }
        [[nodiscard]] constexpr usize size() const noexcept { return N; }
    };

    // --- endian struct (enum values + type tags for template usage) --------------
    struct endian {
        enum v : unsigned char { _little = 0, _big = 1 };
        template<v E>
        struct tag { static constexpr v value = E; };
        using little = tag<_little>;
        using big    = tag<_big>;
    };

    // --- details (internal helpers) ------------------------------------------------
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

        template<endian::v O, std::integral T, fixed_string Str>
            requires (Str.size() > 0 && sizeof(T) <= 8 && Str.size() <= sizeof(T))
        consteval T make_istr_impl() noexcept
        {
            constexpr auto N = Str.size();
            T v{};
            for ( size_t i = 0; i < N; ++i )
            {
                auto s = static_cast<size_t>( O == endian::_little ? i : ( sizeof(T) - 1 - i ) );
                v |= static_cast<T>( static_cast<unsigned char>( Str.data[i] ) )
                    << ( s * 8 );
            }
            return v;
        }

        template<size_t N>
        struct istr_type_selector {};
        template<> struct istr_type_selector<1> { using type = u8;  };
        template<> struct istr_type_selector<2> { using type = u16; };
        template<size_t N> requires (N >= 3 && N <= 4)
        struct istr_type_selector<N> { using type = u32; };
        template<size_t N> requires (N >= 5 && N <= 8)
        struct istr_type_selector<N> { using type = u64; };

        template<size_t N>
        using istr_type_t = typename istr_type_selector<N>::type;
    }

    // --- str / str_type -----------------------------------------------------------
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

    // --- istr_t (variable template with optional positional args) ------------------
    template<fixed_string Str, typename... Args>
        requires (Str.size() > 0 && Str.size() <= 8 && sizeof...(Args) <= 2)
    struct istr_t;

    template<fixed_string Str>
    struct istr_t<Str> {
        static constexpr auto value = details::make_istr_impl<endian::_little, details::istr_type_t<Str.size()>, Str>();
    };

    template<fixed_string Str, typename A>
        requires (Str.size() > 0 && Str.size() <= 8)
    struct istr_t<Str, A> {
        static constexpr auto value = [] {
            if constexpr (std::integral<A>)
                return details::make_istr_impl<endian::_little, A, Str>();
            else
                return details::make_istr_impl<A::value, details::istr_type_t<Str.size()>, Str>();
        }();
    };

    template<fixed_string Str, std::integral T, typename E>
        requires (Str.size() > 0 && sizeof(T) <= 8 && Str.size() <= sizeof(T))
    struct istr_t<Str, T, E> {
        static constexpr auto value = details::make_istr_impl<E::value, T, Str>();
    };

    template<fixed_string Str, typename... Args>
    constexpr auto istr = istr_t<Str, Args...>::value;

    // --- vstr ---------------------------------------------------------------------
    //   vstr<"PE">        -> byte_block<2>
    //   vstr<"PE", 4>     -> byte_block<4> (zero-padded)
    template<fixed_string Str, size_t N = Str.size()>
        requires (N >= Str.size())
    constexpr byte_block<N> vstr = [] {
        byte_block<N> blk{};
        for ( size_t i = 0; i < Str.size(); ++i )
            blk._[i] = static_cast<u8>( static_cast<unsigned char>( Str.data[i] ) );
        return blk;
    }();
}

