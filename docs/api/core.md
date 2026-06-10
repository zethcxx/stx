# core.hpp

## `version_info` / `version`

Semantic version metadata.

| Member | Type | Description |
|--------|------|-------------|
| `version.major` | `int` | Major version |
| `version.minor` | `int` | Minor version |
| `version.patch` | `int` | Patch version |

```cpp
inline constexpr version_info version { 1, 0, 0 };
```

```cpp
if (stx::version.major >= 1) { /* ... */ }
```

---

## Type Aliases

Fixed-width and platform-consistent aliases for primitive types.

| Category | Alias | Underlying Type |
|----------|-------|-----------------|
| Unsigned | `u8` | `std::uint8_t` |
| | `u16` | `std::uint16_t` |
| | `u32` | `std::uint32_t` |
| | `u64` | `std::uint64_t` |
| Signed | `i8` | `std::int8_t` |
| | `i16` | `std::int16_t` |
| | `i32` | `std::int32_t` |
| | `i64` | `std::int64_t` |
| Float | `f32` | `float` |
| | `f64` | `double` |
| Pointer-width | `uptr` | `std::uintptr_t` |
| | `iptr` | `std::intptr_t` |
| Size/Diff | `usize` | `std::size_t` |
| | `isize` | `std::ptrdiff_t` |
| Char | `uchar` | `unsigned char` |

```cpp
stx::u32 val = 0xDEADBEEF;
stx::u8  buf[4];
stx::usize len = sizeof(buf);  // 4
```

---

## Strong Types

Type-safe wrappers that prevent implicit mixing of semantically distinct numeric domains. Each exposes `.get()` to retrieve the raw value and supports arithmetic that preserves the wrapper type.

| Type | Wraps | Domain |
|------|-------|--------|
| `off_s` | `std::ptrdiff_t` | Byte offset |
| `rva_s` | `u32` | Relative virtual address |
| `va_s` | `uptr` | Virtual address |

```cpp
stx::off_s offset{128};
stx::rva_s rva{0x2000};
stx::va_s  base{0x140000000};
```

### Construction and Access

```cpp
stx::off_s off{0x100};          // from raw value
auto raw = off.get();           // std::ptrdiff_t = 256
```

### Arithmetic

Operations with raw integrals return the strong type; operations between
two strong types return the difference as the underlying type.

```cpp
auto advanced = off + 32;        // off_s{288}
auto stepped  = off_s{200} - off_s{150};  // std::ptrdiff_t = 50
```

### Usage Pattern

Strong types serve as documentation and safety layers in pointer and cursor
APIs — `operator[]` and `operator>>` on `ptr<T>` behave differently when
given an `off_s` (byte-level) vs a raw integral (element-level).

```cpp
stx::ptr<int> p{buf};
auto a = p[2];          // element at index 2 (stride-aware)
auto b = p[stx::off_s{8}];  // element at byte offset 8
```

---

## Concepts

Compile-time constraints that gate template overloads.

### `address_like`

Any type that can represent a memory address: raw pointers, `uptr`, `iptr`, `va_s`, or any type with a `.addr()` member returning `uptr`.

```cpp
template<typename Type>
concept address_like = /* ... */;
```

```cpp
static_assert(stx::address_like<int*>);
static_assert(stx::address_like<stx::uptr>);
static_assert(stx::address_like<stx::va_s>);
static_assert(!stx::address_like<float>);
```

### `binary_readable`

Types safe for raw binary copy via `memcpy`: trivially copyable, standard layout, non-empty, non-pointer.

```cpp
template<class Type>
concept binary_readable = /* ... */;
```

```cpp
static_assert(stx::binary_readable<stx::u32>);
static_assert(stx::binary_readable<float>);
static_assert(!stx::binary_readable<std::string>);
static_assert(!stx::binary_readable<int*>);
```

### `byte_swappable`

Integral and enum types suitable for byte-swapping — excludes `bool`, `char`, `wchar_t`, `char8_t/16_t/32_t`.

```cpp
template<typename T>
concept byte_swappable = /* ... */;
```

```cpp
static_assert(stx::byte_swappable<stx::u16>);
static_assert(stx::byte_swappable<stx::i32>);
static_assert(stx::byte_swappable<MyEnum>);
static_assert(!stx::byte_swappable<char>);
static_assert(!stx::byte_swappable<bool>);
```

### `contiguous_buffer` / `writable_buffer`

Ranges exposing `std::data` and `std::size` with trivially copyable elements.
`contiguous_buffer` covers read-only ranges; `writable_buffer` covers mutable ones.

```cpp
template<typename R>
concept contiguous_buffer = /* ... */;

template<typename R>
concept writable_buffer = /* ... */;
```

```cpp
std::vector<stx::u32> vec(1024);
static_assert(stx::contiguous_buffer<decltype(vec)>);

std::array<std::byte, 64> arr;
static_assert(stx::writable_buffer<decltype(arr)>);

// Raw arrays also work
stx::u8 raw[256];
static_assert(stx::writable_buffer<decltype(raw)>);
```

### `buffer_type`

Byte-like element types for `memcur<ByteType>` — byte-wide, trivially copyable, excludes `bool`, `void`, pointers.

```cpp
template<typename T>
concept buffer_type = /* ... */;
```

```cpp
static_assert(stx::buffer_type<char>);
static_assert(stx::buffer_type<const std::byte>);
static_assert(stx::buffer_type<stx::u8>);
static_assert(stx::buffer_type<stx::i8>);
static_assert(!stx::buffer_type<int>);
static_assert(!stx::buffer_type<bool>);
static_assert(!stx::buffer_type<void>);
static_assert(!stx::buffer_type<int*>);
```

### `bounded_array`

C-style bounded arrays of `binary_readable` elements. Used with `ptr::pop<U>()` / `ptr::read<U>()` to return `std::array`.

```cpp
template<typename T>
concept bounded_array = /* ... */;
```

```cpp
static_assert(stx::bounded_array<stx::u32[4]>);
static_assert(stx::bounded_array<char[16]>);
static_assert(!stx::bounded_array<stx::u32[]>);
```

```cpp
stx::ptr<stx::u8> p{data};
auto arr = p.pop<stx::u32[4]>();  // returns std::array<u32, 4>
```

---

## Compile-Time Gap

Sum of `sizeof` each type in the pack.

```cpp
template<typename... Args>
inline constexpr off_s gap_v;
```

```cpp
constexpr auto sz = stx::gap_v<stx::u32, stx::u64, stx::u16>;  // 4 + 8 + 2 = 14
static_assert(sz.get() == 14);
```

Useful for compile-time struct layout calculations:

```cpp
template<typename... Members>
struct packed {
    std::array<std::byte, stx::gap_v<Members...>.get()> storage;
};
```

---

## Casting Helpers

Thin wrappers over standard C++ casts for grep-ability. Every codebase that
uses `reinterpret_cast` heavily benefits from being able to `grep` for
`rcast` instead.

| Helper | Equivalent to |
|--------|---------------|
| `rcast<T>(v)` | `reinterpret_cast<T>(v)` |
| `scast<T>(v)` | `static_cast<T>(v)` |
| `bcast<T>(v)` | `bit_cast<T>(v)` |
| `ccast<T>(v)` | `const_cast<T>(v)` |
| `dcast<T>(v)` | `dynamic_cast<T>(v)` |

```cpp
auto ptr = stx::rcast<stx::u32*>(0x140000000_uptr);
auto truncated = stx::scast<stx::u8>(0x1234);   // 0x34
auto bits = stx::bcast<float>(0x40490FDB_u32);   // 3.14159...
```

---

## `normalize_addr`

Converts any `address_like` type to `uptr` for uniform address arithmetic.

```cpp
template<address_like Addr>
[[nodiscard]] constexpr uptr normalize_addr(Addr base) noexcept;
```

```cpp
int x;
stx::uptr a = stx::normalize_addr(&x);             // address of x
stx::uptr b = stx::normalize_addr(stx::va_s{0x1000});  // 0x1000
stx::uptr c = stx::normalize_addr(stx::null);       // 0
```

Used internally by `mem::read`/`mem::write` to accept any pointer-like type.

---

## `null_t` / `null`

A null constant distinct from `nullptr` — does not satisfy `address_like`,
preventing accidental API misuse with functions that expect real addresses.

```cpp
struct null_t { /* ... */ };
inline constexpr null_t null{};
```

```cpp
stx::ptr<int> p{stx::null};
if (p == stx::null) { /* null case */ }

// stx::normalize_addr(stx::null) == 0

// Does NOT satisfy address_like, so it won't match APIs expecting addresses
// stx::mem::read<int>(stx::null, stx::off_s{});  // compile error
```

