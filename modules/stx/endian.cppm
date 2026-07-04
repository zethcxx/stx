module;

#define STX_MODULE_BUILD
#include "lbyte/stx/endian.hpp"
#include <functional>
#if __has_include(<format>)
    #include <format>
#endif

export module lbyte.stx.endian;

export namespace lbyte::stx::endian
{
    using ::lbyte::stx::endian::order;
    using ::lbyte::stx::endian::compatible;
    using ::lbyte::stx::endian::endian_value;
    using ::lbyte::stx::endian::le;
    using ::lbyte::stx::endian::be;
    using ::lbyte::stx::endian::is_endian_value_v;
}

// --- std::hash --------------------------------------------------------------------

template<lbyte::stx::endian::compatible T, lbyte::stx::endian::order O>
struct std::hash<lbyte::stx::endian::endian_value<T, O>>
{
    [[nodiscard]] constexpr std::size_t operator()(const lbyte::stx::endian::endian_value<T, O>& v) const noexcept {
        return std::hash<T>{}( static_cast<T>(v) );
    }
};

// --- std::formatter ---------------------------------------------------------------

#if __has_include(<format>)
    template<lbyte::stx::endian::compatible T, lbyte::stx::endian::order O>
    struct std::formatter<lbyte::stx::endian::endian_value<T, O>> : std::formatter<T>
    {
        auto format(const lbyte::stx::endian::endian_value<T, O>& v, auto& ctx) const {
            return std::formatter<T>::format(static_cast<T>(v), ctx);
        }
    };
#endif
