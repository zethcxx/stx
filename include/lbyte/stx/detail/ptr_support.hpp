#pragma once

#include <functional>

template<typename T>
struct std::hash<lbyte::stx::ptr<T>>
{
    [[nodiscard]] constexpr auto operator()( const lbyte::stx::ptr<T>& p ) const noexcept {
        return std::hash<lbyte::stx::uptr>{}( p.addr() );
    }
};

#if __has_include(<format>)
    #include <format>

    template<typename T>
    struct std::formatter<lbyte::stx::ptr<T>> {
        char fill_buf[32]{};
        size_t fill_len = 0;
        char align = 0;
        int width = 0;
        bool hash = false;
        char type = 'P';

        constexpr auto parse(auto& pc) {
            auto it = pc.begin();
            auto end = pc.end();

            const char* align_pos = nullptr;
            for (auto p = it; p != end; ++p) {
                auto c = *p;
                if (c == '<' || c == '>' || c == '^')
                    align_pos = p;
            }

            if (align_pos) {
                fill_len = static_cast<size_t>(align_pos - it);
                if (fill_len > 31) fill_len = 31;
                for (size_t i = 0; i < fill_len; ++i)
                    fill_buf[i] = it[i];
                align = *align_pos;
                it = align_pos + 1;
            } else {
                fill_len = 0;
                align = 0;
            }

            width = 0;
            while (it != end && *it >= '0' && *it <= '9') {
                width = width * 10 + (*it - '0');
                ++it;
            }

            hash = (it != end && *it == '#');
            if (hash) ++it;

            type = 0;
            if (it != end && *it != '}') {
                type = *it;
                ++it;
            }
            if (!type) type = 'P';

            return it;
        }

        auto format(const lbyte::stx::ptr<T>& p, auto& ctx) const {
            if (!p.addr())
                return std::format_to(ctx.out(), "null");

            auto addr = p.addr();
            char val_buf[80]{};
            char* val_end = val_buf;
            char prefix[4]{};
            int prefix_len = 0;

            switch (type) {
            case 'P':
                prefix[0] = '0'; prefix[1] = 'x'; prefix_len = 2;
                val_end = std::format_to(val_buf, "{:X}", addr);
                break;
            case 'p':
                prefix[0] = '0'; prefix[1] = 'x'; prefix_len = 2;
                val_end = std::format_to(val_buf, "{:x}", addr);
                break;
            case 'x':
                if (hash) { prefix[0] = '0'; prefix[1] = 'x'; prefix_len = 2; }
                val_end = std::format_to(val_buf, "{:x}", addr);
                break;
            case 'X':
                if (hash) { prefix[0] = '0'; prefix[1] = 'x'; prefix_len = 2; }
                val_end = std::format_to(val_buf, "{:X}", addr);
                break;
            case 'b':
                if (hash) { prefix[0] = '0'; prefix[1] = 'b'; prefix_len = 2; }
                val_end = std::format_to(val_buf, "{:b}", addr);
                break;
            case 'o':
                if (hash) { prefix[0] = '0'; prefix_len = 1; }
                val_end = std::format_to(val_buf, "{:o}", addr);
                break;
            case 'd':
                val_end = std::format_to(val_buf, "{}", addr);
                break;
            default:
                prefix[0] = '0'; prefix[1] = 'X'; prefix_len = 2;
                val_end = std::format_to(val_buf, "{:X}", addr);
                break;
            }

            size_t val_len = static_cast<size_t>(val_end - val_buf);
            size_t total = static_cast<size_t>(prefix_len) + val_len;
            auto out = ctx.out();

            if (width > 0 && total < static_cast<size_t>(width)) {
                size_t pad = static_cast<size_t>(width) - total;
                size_t left = 0, right = 0;

                if (align == '>' || align == 0) {
                    left = pad;
                } else if (align == '<') {
                    right = pad;
                } else {
                    left = pad / 2;
                    right = pad - left;
                }

                auto write_fill = [&](size_t n) {
                    if (fill_len > 0) {
                        for (size_t i = 0; i < n; ++i)
                            *out++ = fill_buf[i % fill_len];
                    } else {
                        for (size_t i = 0; i < n; ++i)
                            *out++ = ' ';
                    }
                };

                write_fill(left);
                for (int i = 0; i < prefix_len; ++i) *out++ = prefix[i];
                for (size_t i = 0; i < val_len; ++i) *out++ = val_buf[i];
                write_fill(right);
            } else {
                for (int i = 0; i < prefix_len; ++i) *out++ = prefix[i];
                for (size_t i = 0; i < val_len; ++i) *out++ = val_buf[i];
            }

            return out;
        }
    };
#endif
