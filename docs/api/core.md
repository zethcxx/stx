# core.hpp

## `version_info` / `version`
```cpp
struct version_info { int major, minor, patch; };
inline constexpr version_info version { 1, 0, 0 };
```

## Type Aliases
```cpp
using u8 = std::uint8_t;   using u16 = std::uint16_t;
using u32 = std::uint32_t; using u64 = std::uint64_t;
using i8 = std::int8_t;    using i16 = std::int16_t;
using i32 = std::int32_t;  using i64 = std::int64_t;
using f32 = float;         using f64 = double;
using usize = std::size_t; using isize = std::ptrdiff_t;
using uptr = std::uintptr_t; using iptr = std::intptr_t;
```

## Strong Types
```cpp
using off_s = details::strong_type<std::ptrdiff_t, details::offset_tag>;
using rva_s = details::strong_type<u32,           details::rva_tag>;
using va_s  = details::strong_type<uptr,          details::va_tag>;
```
```cpp
stx::off_s off{128};
stx::rva_s rva{0x2000};
auto val = off.get();            // 128
auto sum = off + 20;             // off_s{148}
auto diff = off_s{200} - off_s{150}; // 50
```

## Concepts

### `address_like`
```cpp
template<typename Type>
concept address_like =
       std::is_pointer_v<Type>
    or std::same_as<std::remove_cv_t<Type>, std::uintptr_t>
    or std::same_as<std::remove_cv_t<Type>, std::intptr_t>
    or std::same_as<std::remove_cvref_t<Type>, va_s>
    or requires(Type t) { { t.addr() } -> std::same_as<uptr>; };
```

### `binary_readable`
```cpp
template<class Type>
concept binary_readable =
       std::is_trivially_copyable_v<Type>
    and std::is_standard_layout_v<Type>
    and not std::is_empty_v<Type>
    and not std::is_pointer_v<Type>;
```

### `byte_swappable`
```cpp
template<typename T>
concept byte_swappable
    =  (std::integral<T> or std::is_enum_v<T>)
    and not std::same_as<std::remove_cvref_t<T>, bool>
    and not std::same_as<std::remove_cvref_t<T>, char>;
```

### `contiguous_buffer` / `writable_buffer`
```cpp
template<typename R>
concept contiguous_buffer
    =  requires (R& r) {
           { std::data(r) } -> std::convertible_to<const void*>;
           { std::size(r) } -> std::convertible_to<usize>;
       }
    && std::is_trivially_copyable_v<
           std::remove_pointer_t<decltype(std::data(std::declval<R&>()))>>;

template<typename R>
concept writable_buffer
    =  requires (R& r) {
           { std::data(r) } -> std::convertible_to<void*>;
           { std::size(r) } -> std::convertible_to<usize>;
       }
    && std::is_trivially_copyable_v<
           std::remove_pointer_t<decltype(std::data(std::declval<R&>()))>>;
```

### `buffer_type`
```cpp
template<typename T>
concept buffer_type
    =  sizeof(T) == 1
    && std::is_trivially_copyable_v<T>
    && not std::is_pointer_v<T>
    && not std::same_as<std::remove_cv_t<T>, bool>
    && not std::same_as<std::remove_cv_t<T>, void>;
```
```cpp
static_assert(stx::buffer_type<char>);
static_assert(stx::buffer_type<const std::byte>);
static_assert(!stx::buffer_type<int>);
static_assert(!stx::buffer_type<bool>);
```

### `bounded_array`
```cpp
template<typename T>
concept bounded_array
    =  std::is_bounded_array_v<T>
    && binary_readable<std::remove_all_extents_t<T>>;
```
```cpp
auto arr = p.pop<u32[4]>(); // returns std::array<u32, 4>
```

## Alignment
```cpp
template<std::unsigned_integral T>
[[nodiscard]] constexpr T align_up(T value, T alignment) noexcept;

template<std::unsigned_integral T>
[[nodiscard]] constexpr T align_down(T value, T alignment) noexcept;
```
```cpp
auto a = stx::align_up(123u, 16u);   // 128
auto d = stx::align_down(123u, 16u); // 112
```

## Compile-Time Gap
```cpp
template<typename... Args>
inline constexpr off_s gap_v;
```
```cpp
constexpr auto sz = stx::gap_v<u32, u64, u16>; // 14
```

## Casting
```cpp
template<class Type> Type rcast(auto value) noexcept;
template<class Type> constexpr Type scast(auto value) noexcept;
template<typename To, typename From> constexpr To bcast(const From& from) noexcept;
template<class Type> Type ccast(auto value) noexcept;
template<class Type> Type dcast(auto value) noexcept;
```
```cpp
auto p = stx::rcast<char*>(0x140000000_uptr);
auto v = stx::scast<int>(3.14);
```

## `normalize_addr`
```cpp
template<address_like Addr>
[[nodiscard]] constexpr uptr normalize_addr(Addr base) noexcept;
```
```cpp
uptr a = stx::normalize_addr(&x);  // pointer
uptr b = stx::normalize_addr(stx::va_s{0x1000}); // strong type
```

## `null_t` / `null`
```cpp
struct null_t {
    constexpr operator uptr()         const noexcept;
    constexpr operator std::nullptr_t() const noexcept;
    explicit constexpr operator bool() const noexcept;
};
inline constexpr null_t null{};
```
```cpp
stx::ptr<int> p{stx::null};
if (p == stx::null) {}
```
