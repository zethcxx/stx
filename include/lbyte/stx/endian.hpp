#pragma once
#include "core.hpp"

#include <bit>
#include <compare>
#include <concepts>
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <type_traits>

namespace lbyte::stx
{
    // --- byte_order ----------------------------------------------------------------

    enum class byte_order : u8 { little, big };

    // --- endian_compatible concept -------------------------------------------------

    template<typename T>
    concept endian_compatible = byte_swappable<T>;

    // --- endian_value<T, Order> ----------------------------------------------------

    template<endian_compatible T, byte_order Order>
    struct endian_value
    {
        static constexpr bool is_native_little = std::endian::native == std::endian::little;
        static constexpr bool needs_swap = (Order == byte_order::little) != is_native_little;

        T store{};

        constexpr endian_value() noexcept = default;

        constexpr endian_value(T v) noexcept
            : store(swap_if(v))
        {}

        template<endian_compatible U>
        constexpr explicit endian_value(endian_value<U, Order> other) noexcept
            : store(swap_if(static_cast<T>(static_cast<U>(other))))
        {}

        [[nodiscard]] constexpr T get() const noexcept {
            return swap_if(store);
        }

        [[nodiscard]] constexpr explicit operator T() const noexcept {
            return get();
        }

        constexpr endian_value& operator=(T v) noexcept {
            store = swap_if(v);
            return *this;
        }

        [[nodiscard]] constexpr T* data() noexcept {
            return &store;
        }

        [[nodiscard]] constexpr const T* data() const noexcept {
            return &store;
        }

        static constexpr T swap_if(T v) noexcept {
            if constexpr (needs_swap) {
                if constexpr (std::is_enum_v<T>) {
                    using UT = std::underlying_type_t<T>;
                    return static_cast<T>(std::byteswap(static_cast<UT>(v)));
                } else {
                    return std::byteswap(v);
                }
            } else {
                return v;
            }
        }

        // --- compound assignment -----------------------------------------------

        #define LBYTE_ENDIAN_COMPOUND(OP) \
        constexpr endian_value& operator OP##=(T v) noexcept requires std::integral<T> { \
            store = swap_if(static_cast<T>(swap_if(store) OP v)); \
            return *this; \
        }

        LBYTE_ENDIAN_COMPOUND(+)
        LBYTE_ENDIAN_COMPOUND(-)
        LBYTE_ENDIAN_COMPOUND(*)
        LBYTE_ENDIAN_COMPOUND(/)
        LBYTE_ENDIAN_COMPOUND(%)
        LBYTE_ENDIAN_COMPOUND(&)
        LBYTE_ENDIAN_COMPOUND(|)
        LBYTE_ENDIAN_COMPOUND(^)

        #undef LBYTE_ENDIAN_COMPOUND

        constexpr endian_value& operator<<=(T v) noexcept requires std::integral<T> {
            store = swap_if(static_cast<T>(swap_if(store) << v));
            return *this;
        }

        constexpr endian_value& operator>>=(T v) noexcept requires std::integral<T> {
            store = swap_if(static_cast<T>(swap_if(store) >> v));
            return *this;
        }

        // --- increment / decrement ---------------------------------------------

        constexpr endian_value& operator++() noexcept requires std::integral<T> {
            store = swap_if(static_cast<T>(swap_if(store) + 1));
            return *this;
        }

        constexpr endian_value operator++(int) noexcept requires std::integral<T> {
            auto tmp = *this;
            ++*this;
            return tmp;
        }

        constexpr endian_value& operator--() noexcept requires std::integral<T> {
            store = swap_if(static_cast<T>(swap_if(store) - 1));
            return *this;
        }

        constexpr endian_value operator--(int) noexcept requires std::integral<T> {
            auto tmp = *this;
            --*this;
            return tmp;
        }

        // --- unary -------------------------------------------------------------

        [[nodiscard]] friend constexpr endian_value operator+(endian_value v) noexcept requires std::integral<T> {
            return endian_value{ +v.get() };
        }

        [[nodiscard]] friend constexpr endian_value operator-(endian_value v) noexcept requires std::integral<T> {
            return endian_value{ -v.get() };
        }

        [[nodiscard]] friend constexpr endian_value operator~(endian_value v) noexcept requires std::integral<T> {
            return endian_value{ ~v.get() };
        }

        // --- binary arithmetic -------------------------------------------------

        #define LBYTE_ENDIAN_BINARY(OP) \
        [[nodiscard]] friend constexpr endian_value operator OP(endian_value lhs, T rhs) noexcept requires std::integral<T> { \
            return endian_value{ static_cast<T>(lhs.get() OP rhs) }; \
        } \
        [[nodiscard]] friend constexpr endian_value operator OP(T lhs, endian_value rhs) noexcept requires std::integral<T> { \
            return endian_value{ static_cast<T>(lhs OP rhs.get()) }; \
        } \
        [[nodiscard]] friend constexpr endian_value operator OP(endian_value lhs, endian_value rhs) noexcept requires std::integral<T> { \
            return endian_value{ static_cast<T>(lhs.get() OP rhs.get()) }; \
        }

        LBYTE_ENDIAN_BINARY(+)
        LBYTE_ENDIAN_BINARY(-)
        LBYTE_ENDIAN_BINARY(*)
        LBYTE_ENDIAN_BINARY(/)
        LBYTE_ENDIAN_BINARY(%)
        LBYTE_ENDIAN_BINARY(&)
        LBYTE_ENDIAN_BINARY(|)
        LBYTE_ENDIAN_BINARY(^)

        #undef LBYTE_ENDIAN_BINARY

        // --- shift -------------------------------------------------------------

        [[nodiscard]] friend constexpr endian_value operator<<(endian_value lhs, T rhs) noexcept requires std::integral<T> {
            return endian_value{ static_cast<T>(lhs.get() << rhs) };
        }

        [[nodiscard]] friend constexpr endian_value operator>>(endian_value lhs, T rhs) noexcept requires std::integral<T> {
            return endian_value{ static_cast<T>(lhs.get() >> rhs) };
        }

        // --- comparison --------------------------------------------------------

        [[nodiscard]] friend constexpr auto operator<=>(endian_value lhs, endian_value rhs) noexcept {
            return lhs.get() <=> rhs.get();
        }

        [[nodiscard]] friend constexpr bool operator==(endian_value lhs, endian_value rhs) noexcept {
            return lhs.get() == rhs.get();
        }

        // --- I/O ---------------------------------------------------------------

        template<typename Char, typename Traits>
        friend std::basic_ostream<Char, Traits>&
        operator<<(std::basic_ostream<Char, Traits>& os, endian_value v) {
            return os << v.get();
        }

        template<typename Char, typename Traits>
        friend std::basic_istream<Char, Traits>&
        operator>>(std::basic_istream<Char, Traits>& is, endian_value& v) {
            T tmp;
            is >> tmp;
            if (is) v = tmp;
            return is;
        }

        // --- swap --------------------------------------------------------------

        friend constexpr void swap(endian_value& a, endian_value& b) noexcept {
            auto tmp = a.store;
            a.store = b.store;
            b.store = tmp;
        }
    };

    // --- le<T>, be<T> aliases -----------------------------------------------------

    template<endian_compatible T>
    using le = endian_value<T, byte_order::little>;

    template<endian_compatible T>
    using be = endian_value<T, byte_order::big>;

    // --- is_endian_value trait ----------------------------------------------------

    namespace detail
    {
        template<typename T>
        struct is_endian_value_impl : std::false_type {};

        template<endian_compatible T, byte_order O>
        struct is_endian_value_impl<endian_value<T, O>> : std::true_type {};
    }

    template<typename T>
    constexpr bool is_endian_value_v = detail::is_endian_value_impl<std::remove_cvref_t<T>>::value;

} // namespace lbyte::stx

// --- std::hash --------------------------------------------------------------------

template<lbyte::stx::endian_compatible T, lbyte::stx::byte_order O>
struct std::hash<lbyte::stx::endian_value<T, O>>
{
    [[nodiscard]] std::size_t operator()(const lbyte::stx::endian_value<T, O>& v) const noexcept {
        return std::hash<T>{}( static_cast<T>(v) );
    }
};

// --- std::formatter ---------------------------------------------------------------

#if __has_include(<format>)
    #include <format>

    template<lbyte::stx::endian_compatible T, lbyte::stx::byte_order O>
    struct std::formatter<lbyte::stx::endian_value<T, O>> : std::formatter<T>
    {
        auto format(const lbyte::stx::endian_value<T, O>& v, auto& ctx) const {
            return std::formatter<T>::format(static_cast<T>(v), ctx);
        }
    };
#endif
