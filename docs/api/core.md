# core.hpp

## `version_info` / `version`

| Member | Type | Description |
|--------|------|-------------|
| `version.major` | `int` | Major version |
| `version.minor` | `int` | Minor version |
| `version.patch` | `int` | Patch version |

```cpp
inline constexpr version_info version { 1, 0, 0 };
```

---

## Type Aliases

| Category | Alias | Underlying |
|----------|-------|------------|
| Unsigned | `u8` / `u16` / `u32` / `u64` | `std::uint8_t` / `16` / `32` / `64` |
| Signed | `i8` / `i16` / `i32` / `i64` | `std::int8_t` / `16` / `32` / `64` |
| Float | `f32` / `f64` | `float` / `double` |
| Pointer-width | `uptr` / `iptr` | `std::uintptr_t` / `std::intptr_t` |
| Size/Diff | `usize` / `isize` | `std::size_t` / `std::ptrdiff_t` |
| Char | `uchar` / `ushort` / `uint` / `ulong` / `ulonglong` | `unsigned char` / `short` / `int` / `long` / `long long` |

```cpp
stx::u32 val = 0xDEADBEEF;
stx::u8  magic[4];
stx::usize len = sizeof(magic);  // 4
stx::uptr addr = stx::rcast<stx::uptr>(&val);
```

---

## Strong Types

Type-safe wrappers preventing implicit mixing of semantically distinct numeric domains.

| Type | Wraps | Tag | Domain |
|------|-------|-----|--------|
| `off_s` | `std::ptrdiff_t` | `offset_tag` | Byte offset |
| `rva_s` | `u32` | `rva_tag` | Relative virtual address |
| `va_s` | `uptr` | `va_tag` | Virtual address |

### Construction

```cpp
stx::off_s off{128};              // from raw
stx::rva_s rva{0x2000};
stx::va_s  base{0x140000000};

stx::rva_s from_off{off};         // cross-ctor: off_s -> rva_s (same tag family)
// stx::va_s bad{off};            // error: va_s uses va_tag (not offset_tag)
```

### Access and Casting

```cpp
auto raw = off.get();             // std::ptrdiff_t = 128
auto as_i64 = off.as<stx::i64>(); // static_cast
stx::i64 direct = off;            // explicit operator i64
```

### Arithmetic

| Expression | Result type | Semantics |
|------------|-------------|-----------|
| `off + 32` | `off_s` | Offset + scalar |
| `off + other` | `off_s` | Offset + offset |
| `32 + off` | `std::ptrdiff_t` | Scalar + offset value |
| `off - other` | `std::ptrdiff_t` | Difference (loses wrapper) |
| `off - 32` | `off_s` | Offset - scalar |
| `32 - off` | `std::ptrdiff_t` | Scalar - offset value |
| `++off` / `--off` | `off_s&` | Pre-increment/decrement |
| `off++` / `off--` | `off_s` | Post-increment/decrement |

```cpp
auto a = off + 32;                // off_s{160}
auto d = stx::off_s{200} - stx::off_s{150};  // ptrdiff_t = 50
++off;                            // off_s{129}
```

### Comparison

```cpp
off_s{10} < off_s{20};            // true
off_s{10} == off_s{10};           // true
```

### Usage in APIs

Strong types select overloads: `ptr::operator[]` and `ptr::operator>>` behave
differently for `off_s` (byte-level) vs raw integral (element-level).

```cpp
stx::ptr<int> p{buf};
p[2];                // element index 2 (stride-aware)
p[stx::off_s{8}];    // byte offset 8
```

---

## Concepts

### `address_like`

Types representable as a memory address.

```cpp
static_assert(stx::address_like<int*>);
static_assert(stx::address_like<stx::uptr>);
static_assert(stx::address_like<stx::va_s>);
struct HasAddr { stx::uptr addr() const { return 0; } };
static_assert(stx::address_like<HasAddr>);
static_assert(!stx::address_like<float>);
```

### `binary_readable`

Types safe for `memcpy` — trivially copyable, standard layout, non-empty, non-pointer.

```cpp
static_assert(stx::binary_readable<stx::u32>);
static_assert(stx::binary_readable<float>);
static_assert(!stx::binary_readable<std::string>);
static_assert(!stx::binary_readable<int*>);
```

### `byte_swappable`

Integral/enum types suitable for byte-swapping — excludes `bool`, `char` variants.

```cpp
static_assert(stx::byte_swappable<stx::u16>);
static_assert(stx::byte_swappable<MyEnum>);
static_assert(!stx::byte_swappable<char>);
static_assert(!stx::byte_swappable<bool>);
```

### `byte_offset`

Strong offset types accepted by memcpy-based APIs.

```cpp
static_assert(stx::byte_offset<stx::off_s>);
static_assert(stx::byte_offset<stx::rva_s>);
static_assert(!stx::byte_offset<stx::va_s>);
static_assert(!stx::byte_offset<int>);
```

### `contiguous_buffer` / `writable_buffer`

Ranges with `std::data` + `std::size`, trivially-copyable elements.

```cpp
std::vector<stx::u32> v;
static_assert(stx::contiguous_buffer<decltype(v)>);
std::array<std::byte, 64> a;
static_assert(stx::writable_buffer<decltype(a)>);
stx::u8 raw[256];
static_assert(stx::writable_buffer<decltype(raw)>);
```

### `buffer_type`

Byte-wide element types for `memcur` — `sizeof == 1`, no `bool`, `void`, pointers.

```cpp
static_assert(stx::buffer_type<char>);
static_assert(stx::buffer_type<const std::byte>);
static_assert(stx::buffer_type<stx::u8>);
static_assert(stx::buffer_type<stx::i8>);
static_assert(!stx::buffer_type<int>);
static_assert(!stx::buffer_type<bool>);
```

### `bounded_array`

C-style bounded arrays of `binary_readable` elements. Used by `ptr::pop<U>()` / `ptr::read<U>()`.

```cpp
static_assert(stx::bounded_array<stx::u32[4]>);
stx::ptr<stx::u8> p{data};
auto arr = p.pop<stx::u32[4]>();  // std::array<u32, 4>
```

---

## `gap_v`

Sum of `sizeof` each type in the pack.

```cpp
template<typename... Args>
inline constexpr off_s gap_v;
```

```cpp
constexpr auto sz = stx::gap_v<stx::u32, stx::u64, stx::u16>;  // off_s{14}
```

---

## `normalize_addr`

Converts any `address_like` to `uptr`.

```cpp
template<address_like Addr>
constexpr uptr normalize_addr(Addr base) noexcept;
```

```cpp
int x;
stx::uptr a = stx::normalize_addr(&x);            // address of x
stx::uptr b = stx::normalize_addr(stx::va_s{0x1000});  // 0x1000
stx::uptr c = stx::normalize_addr(stx::null);     // 0
```

Used internally by `mem::read`/`mem::write` to accept any pointer-like type.

---

## Casting Helpers

| Helper | Equivalent to | Grep target |
|--------|---------------|-------------|
| `rcast<T>(v)` | `reinterpret_cast<T>(v)` | `rcast` |
| `scast<T>(v)` | `static_cast<T>(v)` | `scast` |
| `bcast<T>(v)` | `bit_cast<T>(v)` | `bcast` |
| `ccast<T>(v)` | `const_cast<T>(v)` | `ccast` |
| `dcast<T>(v)` | `dynamic_cast<T>(v)` | `dcast` |

```cpp
auto ptr = stx::rcast<stx::u32*>(0x140000000_uptr);
auto truncated = stx::scast<stx::u8>(0x1234);
auto bits = stx::bcast<float>(0x40490FDB_u32);   // 3.14159...
```

---

## `defer`

Scope guard — executes a callable on scope exit. Cancelable, non-copyable, non-movable.

```cpp
template<std::invocable<> F>
struct defer { /* ... */ };
template<std::invocable<> F> defer(F) -> defer<F>;
```

```cpp
void* buf = malloc(1024);
stx::defer cleanup{[buf] { free(buf); }};
// ... use buf ...
// cleanup runs automatically at scope exit

// Cancel if no longer needed:
cleanup.cancel();
```

---

## `null_t` / `null`

A null constant distinct from `nullptr`. Does NOT satisfy `address_like`,
preventing accidental API misuse.

```cpp
inline constexpr null_t null{};
```

### Key properties

| Expression | Result |
|------------|--------|
| `null << expr` | `null` (discards `expr`, suppresses `[[nodiscard]]`) |
| `static_cast<uptr>(null)` | `0` |
| `static_cast<bool>(null)` | `false` |
| `static_cast<std::nullptr_t>(null)` | `nullptr` |

### Null as discard accumulator

When calling `[[nodiscard]]` functions like `pop()`, chaining via
`null <<` suppresses the warning and discards the value:

```cpp
stx::ptr<stx::u8> p{data};

// Without null: each pop returns a value we don't need
// stx::null << p.pop<stx::u32>();  // read u32, discard, advance
// stx::null << p.pop<stx::u16>();  // read u16, discard, advance

struct Header {
    stx::u32 magic;
    stx::u16 version;
};
auto hdr = Header{
    .magic   = p.pop<stx::u32>(),   // want these
    .version = p.pop<stx::u16>(),
};
```

### Null with `ptr`

```cpp
stx::ptr<int> p{stx::null};        // null pointer
if (p == stx::null) {}              // comparison
if (p) {}                           // bool conversion works too
```
