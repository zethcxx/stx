#pragma once
#include "core.hpp"
#include <array>
#include <tuple>
#include <string>
#include <string_view>

#if __has_include(<ctre.hpp>)
#include <ctre.hpp>
#endif

namespace lbyte::stx::ct
{
    template<size_t N>
    struct fixed_string {
        using value_type = char;
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

    // --- byte_block (alias to std::array<u8, N>) ----------------------------------
    template<size_t N>
    using byte_block = std::array<u8, N>;

    // --- forward decls for str_type / fmt ----------------------------------------
    template<auto... Vs>
    struct args {};

    template<fixed_string Str, typename... Flags>
    struct str_type;

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

        // --- args helpers --------------------------------------------------------
        template<typename T>
        struct is_args : std::false_type {};
        template<auto... Vs>
        struct is_args<::lbyte::stx::ct::args<Vs...>> : std::true_type {};

        template<typename... Flags>
        constexpr bool has_args_v = (is_args<Flags>::value || ...);

        // Extract args<Vs...> from a flag pack
        template<typename...>
        struct extract_args;
        template<auto... Vs, typename... Rest>
        struct extract_args<::lbyte::stx::ct::args<Vs...>, Rest...> {
            using type = ::lbyte::stx::ct::args<Vs...>;
        };
        template<typename F, typename... Rest>
        struct extract_args<F, Rest...> : extract_args<Rest...> {};
        template<>
        struct extract_args<> {
            using type = void;
        };

        // --- numeric to string ---------------------------------------------------
        template<std::integral T>
        [[nodiscard]] consteval size_t num_str_size(T v, unsigned base) noexcept {
            if (v == 0) return 1;
            size_t n = 0;
            auto uv = static_cast<std::make_unsigned_t<T>>(v);
            while (uv > 0) { uv /= static_cast<std::make_unsigned_t<T>>(base); ++n; }
            return n;
        }

        template<std::integral T>
        consteval void fmt_num(char* buf, T v, unsigned base, bool upper) noexcept {
            if (v == 0) { buf[0] = '0'; return; }
            auto uv = static_cast<std::make_unsigned_t<T>>(v);
            char* p = buf;
            while (uv > 0) {
                auto d = static_cast<unsigned>(uv % static_cast<std::make_unsigned_t<T>>(base));
                *p++ = static_cast<char>(d < 10 ? '0' + d : (upper ? 'A' : 'a') + d - 10);
                uv /= static_cast<std::make_unsigned_t<T>>(base);
            }
            for (size_t i = 0, j = p - buf - 1; i < j; ++i, --j) {
                auto t = buf[i]; buf[i] = buf[j]; buf[j] = t;
            }
        }

        // --- format spec parser --------------------------------------------------
        struct fmt_spec {
            char fill = ' ';
            char align = '>';
            size_t width = 0;
            char type = 'd';
        };

        [[nodiscard]] consteval fmt_spec parse_spec(std::string_view sv) noexcept {
            fmt_spec spec;
            if (sv.empty()) return spec;
            size_t i = 0;
            if (sv[0] == ':') ++i; // format separator, not a fill char
            if (i + 1 < sv.size() && (sv[i + 1] == '<' || sv[i + 1] == '>' || sv[i + 1] == '^')) {
                spec.fill = sv[i];
                spec.align = sv[i + 1];
                i += 2;
            } else if (i < sv.size() && (sv[i] == '<' || sv[i] == '>' || sv[i] == '^')) {
                spec.align = sv[i];
                ++i;
            }
            while (i < sv.size() && sv[i] >= '0' && sv[i] <= '9') {
                spec.width = spec.width * 10 + static_cast<size_t>(sv[i] - '0');
                ++i;
            }
            if (i < sv.size()) spec.type = sv[i];
            return spec;
        }

        template<std::integral T>
        [[nodiscard]] consteval size_t fmt_arg_size(T v, const fmt_spec& spec) noexcept {
            unsigned base = 10;
            switch (spec.type) {
                case 'x': case 'X': base = 16; break;
                case 'o': base = 8; break;
                case 'b': case 'B': base = 2; break;
                case 'c': return 1;
                default: break;
            }
            auto s = num_str_size(v, base);
            return s > spec.width ? s : spec.width;
        }

        template<std::integral T>
        consteval void fmt_arg_write(char* buf, T v, const fmt_spec& spec) noexcept {
            unsigned base = 10;
            bool upper = false;
            switch (spec.type) {
                case 'x': base = 16; break;
                case 'X': base = 16; upper = true; break;
                case 'o': base = 8; break;
                case 'b': case 'B': base = 2; break;
                case 'c': buf[0] = static_cast<char>(v); return;
                default: break;
            }
            auto n = num_str_size(v, base);
            char tmp[64]{};
            fmt_num(tmp, v, base, upper);

            size_t pad = (n < spec.width) ? spec.width - n : 0;
            size_t left = 0, right = 0;
            if (spec.align == '<')      { left = 0; right = pad; }
            else if (spec.align == '^') { left = pad / 2; right = pad - left; }
            else                        { left = pad; right = 0; }

            for (size_t i = 0; i < left; ++i) buf[i] = spec.fill;
            for (size_t i = 0; i < n; ++i) buf[left + i] = tmp[i];
            for (size_t i = 0; i < right; ++i) buf[left + n + i] = spec.fill;
        }

        // --- format string expansion helpers -----------------------------------
        template<typename T>
        struct fmt_helper;

        template<std::integral T>
        struct fmt_helper<T> {
            static consteval size_t expanded_size(const fmt_spec& spec, T v) noexcept {
                return fmt_arg_size(v, spec);
            }
            static consteval void write_to(char* buf, T v, const fmt_spec& spec) noexcept {
                fmt_arg_write(buf, v, spec);
            }
        };

        template<size_t N>
        struct fmt_helper<fixed_string<N>> {
            static consteval size_t expanded_size(const fmt_spec&, auto) noexcept {
                return N;
            }
            static consteval void write_to(char* buf, fixed_string<N> v, const fmt_spec&) noexcept {
                for (size_t i = 0; i < N; ++i) buf[i] = v.data[i];
            }
        };

        template<size_t N>
        struct fmt_helper<std::array<char, N>> {
            static consteval size_t expanded_size(const fmt_spec&, auto) noexcept {
                return N;
            }
            static consteval void write_to(char* buf, const std::array<char, N>& arr, const fmt_spec&) noexcept {
                for (size_t i = 0; i < N; ++i) buf[i] = arr[i];
            }
        };

        // Compute size of expanded format string
        template<fixed_string Str, auto... Vs>
        [[nodiscard]] consteval size_t compute_expanded_size(const args<Vs...>&) noexcept {
            constexpr auto sv = std::string_view{Str.data, Str.size()};
            size_t total = 0;
            size_t ph = 0;
            auto expand_one = [&](auto v, auto spec) {
                using VT = decltype(v);
                total += fmt_helper<VT>::expanded_size(spec, v);
            };
            for (size_t i = 0; i < sv.size(); ) {
                if (sv[i] == '{' && i + 1 < sv.size() && sv[i + 1] == '{') {
                    total += 1; i += 2;
                } else if (sv[i] == '}' && i + 1 < sv.size() && sv[i + 1] == '}') {
                    total += 1; i += 2;
                } else if (sv[i] == '{') {
                    auto end = sv.find('}', i + 1);
                    auto spec_str = sv.substr(i + 1, end - i - 1);
                    auto spec = parse_spec(spec_str);
                    [&]<size_t... Is>(std::index_sequence<Is...>) {
                        ((ph == Is ? (expand_one(std::get<Is>(std::tuple{Vs...}), spec), 0) : 0), ...);
                    }(std::make_index_sequence<sizeof...(Vs)>{});
                    ++ph;
                    i = end + 1;
                } else {
                    total += 1; ++i;
                }
            }
            return total;
        }

        // Fill expanded format string into buffer
        template<fixed_string Str, size_t TotalSize, auto... Vs>
        [[nodiscard]] consteval auto expand_format_fill(const args<Vs...>&) noexcept
            -> std::array<char, TotalSize>
        {
            constexpr auto sv = std::string_view{Str.data, Str.size()};
            std::array<char, TotalSize> result{};
            size_t dst = 0;
            size_t ph = 0;
            auto write_one = [&](auto v, auto spec) {
                using VT = decltype(v);
                fmt_helper<VT>::write_to(result.data() + dst, v, spec);
                dst += fmt_helper<VT>::expanded_size(spec, v);
            };
            for (size_t i = 0; i < sv.size(); ) {
                if (sv[i] == '{' && i + 1 < sv.size() && sv[i + 1] == '{') {
                    result[dst++] = '{'; i += 2;
                } else if (sv[i] == '}' && i + 1 < sv.size() && sv[i + 1] == '}') {
                    result[dst++] = '}'; i += 2;
                } else if (sv[i] == '{') {
                    auto end = sv.find('}', i + 1);
                    auto spec_str = sv.substr(i + 1, end - i - 1);
                    auto spec = parse_spec(spec_str);
                    [&]<size_t... Is>(std::index_sequence<Is...>) {
                        ((ph == Is ? (write_one(std::get<Is>(std::tuple{Vs...}), spec), 0) : 0), ...);
                    }(std::make_index_sequence<sizeof...(Vs)>{});
                    ++ph;
                    i = end + 1;
                } else {
                    result[dst++] = sv[i++];
                }
            }
            return result;
        }

        // Partial specialization captures Vs... for expansion
        template<fixed_string Str, typename>
        struct expand_format_impl;

        template<fixed_string Str, auto... Vs>
        struct expand_format_impl<Str, ::lbyte::stx::ct::args<Vs...>> {
            static constexpr size_t total_size = compute_expanded_size<Str>(::lbyte::stx::ct::args<Vs...>{});
            using array_type = std::array<char, total_size>;
            static consteval array_type fill() noexcept {
                return expand_format_fill<Str, total_size>(::lbyte::stx::ct::args<Vs...>{});
            }
        };

        // --- apply flags to array (recursive; size may change per step) ----------
        template<size_t N>
        [[nodiscard]] consteval auto apply_flags(std::array<char, N> data) noexcept
        {
            return data;
        }

        template<typename F, typename... Rest, size_t N>
        [[nodiscard]] consteval auto apply_flags(std::array<char, N> data) noexcept
        {
            auto r = F::apply(data);
            if constexpr (sizeof...(Rest) == 0)
                return r;
            else
                return apply_flags<Rest...>(r);
        }

        // --- NTTP-based chain (forces materialization via template instantiation) -
        template<auto Data>
        struct apply_chain_base {
            using type = decltype(Data);
            static constexpr type value = Data;
        };

        template<auto Data, typename... Flags>
        struct apply_chain;

        template<auto Data>
        struct apply_chain<Data> : apply_chain_base<Data> {};

        template<auto Data, typename F, typename... Rest>
        struct apply_chain<Data, F, Rest...>
            : apply_chain<F::apply(Data), Rest...> {};

        // --- fixed_string from array ---------------------------------------------
        template<size_t N>
        [[nodiscard]] consteval auto arr_to_fs(const std::array<char, N>& arr) noexcept
            -> fixed_string<N>
        {
            fixed_string<N> fs{};
            for (size_t i = 0; i < N; ++i)
                fs.data[i] = arr[i];
            return fs;
        }

        // --- exact-size / pad helpers --------------------------------------------
        template<size_t N>
        [[nodiscard]] constexpr size_t arr_content_len(const std::array<char, N>& data) noexcept {
            size_t n = 0;
            while (n < N && data[n] != '\0') ++n;
            return n;
        }

        // Shrink a null-terminated array to exactly its content (content + '\0').
        template<auto Data>
        [[nodiscard]] consteval auto shrink_nttp() noexcept {
            constexpr size_t len = arr_content_len(Data);
            std::array<char, len + 1> out{};
            for (size_t i = 0; i < len; ++i)
                out[i] = Data[i];
            return out;
        }

        // Force exactly Size bytes: content capped at Size - 1, always
        // null-terminated, remaining bytes zeroed.
        template<size_t Size, size_t N>
        [[nodiscard]] consteval auto fixed_arr(std::array<char, N> data) noexcept
            -> std::array<char, Size>
        {
            static_assert(Size >= 1, "ct::fmt::fixed requires Size >= 1");
            std::array<char, Size> out{};
            size_t len = arr_content_len(data);
            for (size_t i = 0; i < len && i < Size - 1; ++i)
                out[i] = data[i];
            out[Size - 1] = '\0';
            return out;
        }

    }

    // --- repeat: repeat a pattern V, N times ------------------------------------
    template<auto V, size_t Reps>
    struct repeat_t;

    template<auto V, size_t Reps>
        requires std::is_scalar_v<std::remove_cvref_t<decltype(V)>>
    struct repeat_t<V, Reps> {
        using value_type = std::remove_cvref_t<decltype(V)>;
        static constexpr std::array<value_type, Reps> value = [] {
            std::array<value_type, Reps> a{};
            for (size_t i = 0; i < Reps; ++i)
                a[i] = V;
            return a;
        }();
    };

    template<auto V, size_t Reps>
        requires (not std::is_scalar_v<decltype(V)>
               && requires { typename std::remove_cvref_t<decltype(V)>::value_type; }
               && requires { std::tuple_size<std::remove_cvref_t<decltype(V)>>{}; })
    struct repeat_t<V, Reps> {
        using element_type = typename std::remove_cvref_t<decltype(V)>::value_type;
        static constexpr size_t pat_size =
            std::tuple_size_v<std::remove_cvref_t<decltype(V)>>;
        static constexpr size_t total = pat_size * Reps;
        static constexpr std::array<element_type, total> value = [] {
            std::array<element_type, total> a{};
            for (size_t r = 0; r < Reps; ++r)
                for (size_t i = 0; i < pat_size; ++i)
                    a[r * pat_size + i] = V[i];
            return a;
        }();
        using value_type = element_type;
    };

    template<auto V, size_t Reps>
    constexpr auto repeat = repeat_t<V, Reps>::value;

    // --- fmt (compile-time string transforms) -----------------------------------
    struct fmt {
        struct strip {
            template<size_t N>
            static consteval auto apply(std::array<char, N> data) noexcept
                -> std::array<char, N>
            { return details::strip_arr(data); }
        };

        struct unindent {
            template<size_t N>
            static consteval auto apply(std::array<char, N> data) noexcept
                -> std::array<char, N>
            { return details::unindent_arr(data); }
        };

        struct trim_left {
            template<size_t N>
            static consteval auto apply(std::array<char, N> data) noexcept
                -> std::array<char, N>
            {
                size_t null_pos = 0;
                while (null_pos < N && data[null_pos] != '\0') ++null_pos;
                std::array<char, N> result{};
                size_t dst = 0;
                size_t line_start = 0;
                for (size_t i = 0; i <= null_pos; ++i) {
                    auto c = (i < null_pos) ? data[i] : '\n';
                    if (c == '\n') {
                        size_t content = line_start;
                        while (content < i && (data[content] == ' ' || data[content] == '\t'))
                            ++content;
                        for (size_t j = content; j < i; ++j)
                            result[dst++] = data[j];
                        if (i < null_pos) result[dst++] = '\n';
                        line_start = i + 1;
                    }
                }
                return result;
            }
        };

        struct trim_right {
            template<size_t N>
            static consteval auto apply(std::array<char, N> data) noexcept
                -> std::array<char, N>
            {
                size_t null_pos = 0;
                while (null_pos < N && data[null_pos] != '\0') ++null_pos;
                if (null_pos == 0) return data;
                std::array<char, N> result{};
                size_t dst = 0;
                size_t line_start = 0;
                for (size_t i = 0; i <= null_pos; ++i) {
                    auto c = (i < null_pos) ? data[i] : '\n';
                    if (c == '\n') {
                        size_t end = dst;
                        while (end > line_start && (result[end - 1] == ' ' || result[end - 1] == '\t'))
                            --end;
                        dst = end;
                        if (i < null_pos) result[dst++] = '\n';
                        line_start = dst;
                    } else {
                        result[dst++] = c;
                    }
                }
                result[dst] = '\0';
                return result;
            }
        };

        struct trim_trailing_lines {
            template<size_t N>
            static consteval auto apply(std::array<char, N> data) noexcept
                -> std::array<char, N>
            {
                size_t null_pos = 0;
                while (null_pos < N && data[null_pos] != '\0') ++null_pos;
                std::array<char, N> result{};
                size_t dst = 0;
                size_t line_start = 0;
                for (size_t i = 0; i <= null_pos; ++i) {
                    if (i == null_pos || data[i] == '\n') {
                        size_t end = i;
                        while (end > line_start && (data[end - 1] == ' ' || data[end - 1] == '\t'))
                            --end;
                        for (size_t j = line_start; j < end; ++j)
                            result[dst++] = data[j];
                        if (i < null_pos) result[dst++] = '\n';
                        line_start = i + 1;
                    }
                }
                return result;
            }
        };

        struct collapse_blank_lines {
            template<size_t N>
            static consteval auto apply(std::array<char, N> data) noexcept
                -> std::array<char, N>
            {
                size_t null_pos = 0;
                while (null_pos < N && data[null_pos] != '\0') ++null_pos;
                std::array<char, N> result{};
                size_t dst = 0;
                bool prev_blank = false;
                for (size_t i = 0; i < null_pos; ++i) {
                    auto c = data[i];
                    if (c == '\n') {
                        if (!prev_blank)
                            result[dst++] = c;
                        prev_blank = true;
                    } else if (c == ' ' || c == '\t') {
                        if (!prev_blank)
                            result[dst++] = c;
                    } else {
                        result[dst++] = c;
                        prev_blank = false;
                    }
                }
                return result;
            }
        };

        struct remove_blank_lines {
            template<size_t N>
            static consteval auto apply(std::array<char, N> data) noexcept
                -> std::array<char, N>
            {
                size_t null_pos = 0;
                while (null_pos < N && data[null_pos] != '\0') ++null_pos;
                std::array<char, N> result{};
                size_t dst = 0;
                size_t line_start = 0;
                bool pending_nl = false;
                for (size_t i = 0; i <= null_pos; ++i) {
                    auto c = (i < null_pos) ? data[i] : '\n';
                    if (c == '\n') {
                        bool blank = true;
                        for (size_t j = line_start; j < i; ++j) {
                            if (data[j] != ' ' && data[j] != '\t') {
                                blank = false;
                                break;
                            }
                        }
                        if (!blank) {
                            if (pending_nl) result[dst++] = '\n';
                            for (size_t j = line_start; j < i; ++j)
                                result[dst++] = data[j];
                            pending_nl = true;
                        }
                        line_start = i + 1;
                    }
                }
                return result;
            }
        };

        struct trim_each_line {
            template<size_t N>
            static consteval auto apply(std::array<char, N> data) noexcept
                -> std::array<char, N>
            {
                size_t null_pos = 0;
                while (null_pos < N && data[null_pos] != '\0') ++null_pos;
                std::array<char, N> result{};
                size_t dst = 0;
                size_t line_start = 0;
                bool leading = true;
                for (size_t i = 0; i <= null_pos; ++i) {
                    auto c = data[i];
                    if (leading) {
                        if (c == '\n') {
                            result[dst++] = c;
                            line_start = dst;
                        } else if (c != ' ' && c != '\t') {
                            result[dst++] = c;
                            leading = false;
                        }
                    } else if (i == null_pos || c == '\n') {
                        size_t end = dst;
                        while (end > line_start && (result[end - 1] == ' ' || result[end - 1] == '\t'))
                            --end;
                        dst = end;
                        if (i < null_pos) result[dst++] = '\n';
                        line_start = dst;
                        leading = true;
                    } else {
                        result[dst++] = c;
                    }
                }
                result[dst] = '\0';
                return result;
            }
        };

        struct collapse_whitespace {
            template<size_t N>
            static consteval auto apply(std::array<char, N> data) noexcept
                -> std::array<char, N>
            {
                size_t null_pos = 0;
                while (null_pos < N && data[null_pos] != '\0') ++null_pos;
                std::array<char, N> result{};
                size_t dst = 0;
                bool prev_ws = false;
                for (size_t i = 0; i < null_pos; ++i) {
                    auto c = data[i];
                    if (c == ' ' || c == '\t') {
                        if (!prev_ws)
                            result[dst++] = ' ';
                        prev_ws = true;
                    } else {
                        result[dst++] = c;
                        prev_ws = false;
                    }
                }
                return result;
            }
        };

        template<fixed_string From, fixed_string To>
            requires (From.size() > 0)
        struct replace_all {
            template<size_t N>
            static consteval auto apply(std::array<char, N> data) noexcept
            {
                constexpr auto from_n = From.size();
                constexpr auto to_n   = To.size();
                size_t null_pos = 0;
                while (null_pos < N && data[null_pos] != '\0') ++null_pos;
                std::array<char, N * (to_n > 0 ? to_n : 1)> result{};
                size_t dst = 0;
                for (size_t i = 0; i < null_pos; ) {
                    bool match = (i + from_n <= null_pos);
                    if (match) {
                        for (size_t j = 0; j < from_n; ++j) {
                            if (data[i + j] != From.data[j]) { match = false; break; }
                        }
                    }
                    if (match) {
                        for (size_t j = 0; j < to_n; ++j)
                            result[dst++] = To.data[j];
                        i += from_n;
                    } else {
                        result[dst++] = data[i++];
                    }
                }
                return result;
            }
        };

        // Force the result to exactly Size bytes. Terminal flag: use it last,
        // otherwise its size persists only if no later flag resizes.
        template<size_t Size>
        struct fixed {
            template<size_t N>
            static consteval auto apply(std::array<char, N> data) noexcept
                -> std::array<char, Size>
            { return details::fixed_arr<Size>(data); }
        };

        // Append N zero bytes after the terminator, growing the buffer.
        // Never truncates, so there is no size to count. Like fixed, this is
        // a layout flag: the extra zeros survive the final shrink.
        template<size_t N = 1>
        struct pad_end {
            static_assert(N >= 1, "ct::fmt::pad_end requires N >= 1");
            template<size_t M>
            static consteval auto apply(std::array<char, M> data) noexcept
                -> std::array<char, M + N>
            {
                std::array<char, M + N> out{};
                for (size_t i = 0; i < M; ++i)
                    out[i] = data[i];
                return out;
            }
        };

        template<fixed_string Marker>
        struct strip_line_comments {
            template<size_t N>
            static consteval auto apply(std::array<char, N> data) noexcept
                -> std::array<char, N>
            {
                constexpr auto mlen = Marker.size();
                size_t null_pos = 0;
                while (null_pos < N && data[null_pos] != '\0') ++null_pos;
                std::array<char, N> result{};
                size_t dst = 0;
                for (size_t i = 0; i < null_pos; ) {
                    if (data[i] == '\n') {
                        result[dst++] = data[i++];
                        continue;
                    }
                    bool is_marker = (i + mlen <= null_pos);
                    if (is_marker) {
                        for (size_t j = 0; j < mlen; ++j) {
                            if (data[i + j] != Marker.data[j]) { is_marker = false; break; }
                        }
                    }
                    if (is_marker) {
                        while (i < null_pos && data[i] != '\n')
                            ++i;
                    } else {
                        result[dst++] = data[i++];
                    }
                }
                return result;
            }
        };

        template<typename... Fs>
        struct chain {
            template<size_t N>
            static consteval auto apply(std::array<char, N> data) noexcept
            { return details::apply_flags<Fs...>(data); }
        };

        // Preset
        using trim_block = chain<strip, unindent>;
    };

    namespace details
    {
        // Detect layout flags (fixed / pad_end): top-level or nested inside a chain
        template<typename F>
        struct is_layout : std::false_type {};
        template<size_t Size>
        struct is_layout<::lbyte::stx::ct::fmt::fixed<Size>> : std::true_type {};
        template<size_t N>
        struct is_layout<::lbyte::stx::ct::fmt::pad_end<N>> : std::true_type {};
        template<typename... Fs>
        struct is_layout<::lbyte::stx::ct::fmt::chain<Fs...>>
            : std::bool_constant<(is_layout<Fs>::value || ...)> {};

        template<typename... Flags>
        constexpr bool has_layout_v = (is_layout<Flags>::value || ...);
    }

    template<fixed_string Str, typename... Flags>
    struct str_type {
    private:
        static constexpr bool _has_args = (details::is_args<Flags>::value || ...);
        static constexpr bool _has_layout = details::has_layout_v<Flags...>;

        static constexpr auto compute_initial() noexcept {
            constexpr size_t N = Str.size() + 1;
            std::array<char, N> arr{};
            for (size_t i = 0; i < N; ++i)
                arr[i] = Str.data[i];
            return arr;
        }

        static constexpr auto _raw_value = [] {
            if constexpr (_has_args) {
                using ArgsT = typename details::extract_args<Flags...>::type;
                return details::shrink_nttp<details::expand_format_impl<Str, ArgsT>::fill()>();
            } else {
                constexpr auto init = compute_initial();
                constexpr auto full = details::apply_chain<init, Flags...>::value;
                if constexpr (_has_layout)
                    return full;
                else
                    return details::shrink_nttp<full>();
            }
        }();

    public:
        static constexpr auto value = _raw_value;

        using char_type = char;
        using value_type = const char*;
        using view_type  = std::string_view;

        [[nodiscard]] constexpr const char* data() const noexcept { return value.data(); }
        [[nodiscard]] constexpr size_t size() const noexcept {
            size_t n = 0;
            while (n < value.size() && value[n]) ++n;
            return n;
        }

        [[nodiscard]] constexpr operator const char*() const noexcept {
            return value.data();
        }

        [[nodiscard]] constexpr operator view_type() const noexcept {
            return {value.data(), size()};
        }

        [[nodiscard]] constexpr std::string str() const {
            return std::string{ value.data(), size() };
        }

        [[nodiscard]] constexpr operator std::string() const {
            return std::string{ value.data(), size() };
        }

        // Structured binding: auto [cstr, len] = ct::str<...>;
        template<size_t I>
            requires (I < 2)
        [[nodiscard]] constexpr auto get() const noexcept {
            if constexpr (I == 0) return value.data();
            else                  return size();
        }

        template<typename... MoreFlags>
        static constexpr auto apply() noexcept {
            constexpr bool more_layout = details::has_layout_v<MoreFlags...>;
            constexpr auto new_fs = []() {
                constexpr auto full = details::apply_chain<_raw_value, MoreFlags...>::value;
                if constexpr (more_layout)
                    return details::arr_to_fs(full);
                else
                    return details::arr_to_fs(details::shrink_nttp<full>());
            }();
            if constexpr (more_layout)
                return str_type<new_fs, fmt::fixed<new_fs.size() + 1>>{};
            else
                return str_type<new_fs>{};
        }
    };

    template<fixed_string Str, typename... Flags>
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

    // --- ct::re (CTRE-based compile-time regex, optional) -------------------------

#if __has_include(<ctre.hpp>)
    template<ctll::fixed_string Pattern>
    struct re
    {
        template<fixed_string Replacement>
        struct replace {
            template<size_t N>
            static consteval auto apply(std::array<char, N> data) noexcept
                -> std::array<char, N>
            {
                constexpr auto repl = Replacement.data;
                constexpr auto repl_n = Replacement.size();

                size_t null_pos = 0;
                while (null_pos < N && data[null_pos] != '\0') ++null_pos;
                if (null_pos == 0) return data;

                std::array<char, N> result{};
                size_t dst = 0;

                const char* ptr = data.data();
                size_t remaining_size = null_pos;

                while (remaining_size > 0)
                {
                    std::string_view current{ptr, remaining_size};
                    auto match = ctre::search<Pattern>(current);
                    if (!match) break;

                    auto view = match.to_view();
                    auto pos = static_cast<size_t>(view.data() - ptr);
                    auto len = view.size();

                    for (size_t i = 0; i < pos && dst < N - 1; ++i)
                        result[dst++] = ptr[i];

                    for (size_t i = 0; i < repl_n && dst < N - 1; ++i)
                        result[dst++] = repl[i];

                    if (len == 0) [[unlikely]] {
                        if (dst < N - 1) result[dst++] = ptr[pos];
                        ptr += pos + 1;
                        remaining_size -= pos + 1;
                    } else {
                        ptr += pos + len;
                        remaining_size -= pos + len;
                    }
                }

                for (size_t i = 0; i < remaining_size && dst < N - 1; ++i)
                    result[dst++] = ptr[i];

                return result;
            }
        };

        struct remove {
            template<size_t N>
            static consteval auto apply(std::array<char, N> data) noexcept
                -> std::array<char, N>
            {
                return replace<"">::apply(data);
            }
        };
    };
#endif

    // --- vstr ---------------------------------------------------------------------
    //   vstr<"PE">        -> byte_block<2>
    //   vstr<"PE", 4>     -> byte_block<4> (zero-padded)
    template<fixed_string Str, size_t N = Str.size()>
        requires (N >= Str.size())
    constexpr byte_block<N> vstr = [] {
        byte_block<N> blk{};
        for ( size_t i = 0; i < Str.size(); ++i )
            blk[i] = static_cast<u8>( static_cast<unsigned char>( Str.data[i] ) );
        return blk;
    }();
}

#include "detail/str_support.hpp"

