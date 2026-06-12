# core.hpp

All examples assume `using namespace stx;` for brevity.

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

## Type Aliases (stx::*)

| Category | Alias | Underlying |
|----------|-------|------------|
| Unsigned | `u8` / `u16` / `u32` / `u64` | `std::uint8_t` / `16` / `32` / `64` |
| Signed | `i8` / `i16` / `i32` / `i64` | `std::int8_t` / `16` / `32` / `64` |
| Float | `f32` / `f64` | `float` / `double` |
| Pointer-width | `uptr` / `iptr` | `std::uintptr_t` / `std::intptr_t` |
| Size/Diff | `usize` / `isize` | `std::size_t` / `std::ptrdiff_t` |
| Char | `uchar` / `ushort` / `uint` / `ulong` / `ulonglong` | `unsigned char` / `short` / `int` / `long` / `long long` |

```cpp
u32 val = 0xDEADBEEF;
u8  magic[4];
usize len = sizeof(magic);  // 4
uptr addr = rcast<uptr>(&val);
```

---

## Strong Types (stx::off_s, stx::rva_s, stx::va_s)

Type-safe wrappers preventing implicit mixing of semantically distinct numeric domains.

| Type | Wraps | Tag | Domain |
|------|-------|-----|--------|
| `off_s` | `std::ptrdiff_t` | `offset_tag` | Byte offset |
| `rva_s` | `u32` | `rva_tag` | Relative virtual address |
| `va_s` | `uptr` | `va_tag` | Virtual address |

### Construction (stx::off_s, stx::rva_s, stx::va_s)

```cpp
off_s off{128};              // from raw
rva_s rva{0x2000};
va_s  base{0x140000000};

rva_s from_off{off};         // cross-ctor: off_s -> rva_s (same tag family)
// va_s bad{off};            // error: va_s uses va_tag (not offset_tag)
```

### Access and Casting (stx::off_s, stx::rva_s, stx::va_s)

```cpp
auto raw = off.get();             // std::ptrdiff_t = 128
auto as_i64 = off.as<i64>(); // static_cast
i64 direct = off;            // explicit operator i64
```

### Arithmetic (stx::off_s, stx::va_s)

| Expression | Result type | Semantics |
|------------|-------------|-----------|
| `off + 32` | `off_s` | Offset + scalar |
| `off + other` (same tag) | `off_s` | Offset + offset |
| `va + off` (cross-tag) | `va_s` | VA + offset = VA (converts `off` to `uptr`) |
| `32 + off` | `std::ptrdiff_t` | Scalar + offset value |
| `off - other` (same tag) | `std::ptrdiff_t` | Difference (loses wrapper) |
| `va - off` (cross-tag) | `va_s` | VA - offset = VA |
| `off - 32` | `off_s` | Offset - scalar |
| `32 - off` | `std::ptrdiff_t` | Scalar - offset value |
| `++off` / `--off` | `off_s&` | Pre-increment/decrement |
| `off++` / `off--` | `off_s` | Post-increment/decrement |

```cpp
auto a = off + 32;                // off_s{160}
auto d = off_s{200} - off_s{150};  // ptrdiff_t = 50
++off;                            // off_s{129}
auto va = va_s{0x1000} + off_s{8};  // va_s{0x1008}
auto va2 = va_s{0x1000} - off_s{4};  // va_s{0xFFC}
```

### Comparison (stx::off_s)

```cpp
off_s{10} < off_s{20};            // true
off_s{10} == off_s{10};           // true
```

### Usage in APIs (stx::off_s)

Strong types select overloads: `ptr::operator>>` accepts `off_s` directly
(byte-level chase). For byte-level displacement, use `ptr + off_s{n}`:

```cpp
ptr<int> p{buf};
p[2];                        // element index 2 (element-level)
p[2, 1];                     // byte offset 2 (custom stride 1)
auto bp = p + off_s{8}; // byte offset 8 via arithmetic
auto v  = (p + off_s{8}).read<u32>(); // read at byte 8
```

---

## Concepts

### `address_like` (concept)

Types representable as a memory address.

```cpp
static_assert(address_like<int*>);
static_assert(address_like<uptr>);
static_assert(address_like<va_s>);
struct HasAddr { uptr addr() const { return 0; } };
static_assert(address_like<HasAddr>);
static_assert(!address_like<float>);
```

### `binary_readable` (concept)

Types safe for `memcpy` — trivially copyable, standard layout, non-empty, non-pointer.

```cpp
static_assert(binary_readable<u32>);
static_assert(binary_readable<float>);
static_assert(!binary_readable<std::string>);
static_assert(!binary_readable<int*>);
```

### `byte_swappable` (concept)

Integral/enum types suitable for byte-swapping — excludes `bool`, `char` variants.

```cpp
static_assert(byte_swappable<u16>);
static_assert(byte_swappable<MyEnum>);
static_assert(!byte_swappable<char>);
static_assert(!byte_swappable<bool>);
```

### `byte_offset` (concept)

Strong offset types accepted by memcpy-based APIs.

```cpp
static_assert(byte_offset<off_s>);
static_assert(byte_offset<rva_s>);
static_assert(!byte_offset<va_s>);
static_assert(!byte_offset<int>);
```

### `contiguous_buffer` / `writable_buffer` (concepts)

Ranges with `std::data` + `std::size`, trivially-copyable elements.

```cpp
std::vector<u32> v;
static_assert(contiguous_buffer<decltype(v)>);
std::array<std::byte, 64> a;
static_assert(writable_buffer<decltype(a)>);
u8 raw[256];
static_assert(writable_buffer<decltype(raw)>);
```

### `buffer_type` (concept)

Byte-wide element types for `memcur` — `sizeof == 1`, no `bool`, `void`, pointers.

```cpp
static_assert(buffer_type<char>);
static_assert(buffer_type<const std::byte>);
static_assert(buffer_type<u8>);
static_assert(buffer_type<i8>);
static_assert(!buffer_type<int>);
static_assert(!buffer_type<bool>);
```

### `bounded_array` (concept)

C-style bounded arrays of `binary_readable` elements. Used by `ptr::pop<U>()` / `ptr::read<U>()`.

```cpp
static_assert(bounded_array<u32[4]>);
ptr<u8> p{data};
auto arr = p.pop<u32[4]>();  // std::array<u32, 4>
```

---

## `normalize_addr` (stx::normalize_addr)

Converts any `address_like` to `uptr`.

```cpp
template<address_like Addr>
constexpr uptr normalize_addr(Addr base) noexcept;
```

```cpp
int x;
uptr a = normalize_addr(&x);            // address of x
uptr b = normalize_addr(va_s{0x1000});  // 0x1000
uptr c = normalize_addr(null);     // 0
```

Used internally by `mem::read`/`mem::write` to accept any pointer-like type.

---

## Casting Helpers (stx::rcast, stx::scast, stx::bcast, etc.)

| Helper | Equivalent to | Grep target |
|--------|---------------|-------------|
| `rcast<T>(v)` | `reinterpret_cast<T>(v)` | `rcast` |
| `scast<T>(v)` | `static_cast<T>(v)` | `scast` |
| `bcast<T>(v)` | `bit_cast<T>(v)` | `bcast` |
| `ccast<T>(v)` | `const_cast<T>(v)` | `ccast` |
| `dcast<T>(v)` | `dynamic_cast<T>(v)` | `dcast` |

```cpp
auto ptr = rcast<u32*>(0x140000000_uptr);
auto truncated = scast<u8>(0x1234);
auto bits = bcast<float>(0x40490FDB_u32);   // 3.14159...
```

---

## `defer` (stx::defer)

Scope guard — executes a callable on scope exit. Cancelable, non-copyable, non-movable.

```cpp
template<std::invocable<> F>
struct defer { /* ... */ };
template<std::invocable<> F> defer(F) -> defer<F>;
```

```cpp
void* buf = malloc(1024);
defer cleanup{[buf] { free(buf); }};
// ... use buf ...
// cleanup runs automatically at scope exit

// Cancel if no longer needed:
cleanup.cancel();
```

---

## `null_t` / `null` (stx::null_t, stx::null)

A null constant distinct from `nullptr`. Does NOT satisfy `address_like`,
preventing accidental API misuse.

```cpp
inline constexpr null_t null{};
```

### Key properties (stx::null_t)

| Expression | Result |
|------------|--------|
| `null << expr` | `null` (discards `expr`, suppresses `[[nodiscard]]`) |
| `static_cast<uptr>(null)` | `0` |
| `static_cast<bool>(null)` | `false` |
| `static_cast<std::nullptr_t>(null)` | `nullptr` |

### Null as discard accumulator (stx::null)

When calling `[[nodiscard]]` functions like `pop()`, chaining via
`null <<` suppresses the warning and discards the value:

```cpp
ptr<u8> p{data};

// Without null: each pop returns a value we don't need
// null << p.pop<u32>();  // read u32, discard, advance
// null << p.pop<u16>();  // read u16, discard, advance

struct Header {
    u32 magic;
    u16 version;
};
auto hdr = Header{
    .magic   = p.pop<u32>(),   // want these
    .version = p.pop<u16>(),
};
```

### Null with `ptr` (stx::null)

```cpp
ptr<int> p{null};        // null pointer
if (p == null) {}              // comparison
if (p) {}                           // bool conversion works too
```
