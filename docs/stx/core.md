# core.hpp

All examples assume `using namespace stx;` for brevity.

## `version_info` / `version`

| Member          | Type  | Description   |
| --------------- | ----- | ------------- |
| `version.major` | `int` | Major version |
| `version.minor` | `int` | Minor version |
| `version.patch` | `int` | Patch version |

```cpp
inline constexpr version_info version { 0, 2, 0 };
```

---

## Type Aliases (stx::*)

| Category      | Alias                                               | Underlying                                               |
| ------------- | --------------------------------------------------- | -------------------------------------------------------- |
| Unsigned      | `u8` / `u16` / `u32` / `u64`                        | `std::uint8_t` / `16` / `32` / `64`                      |
| Signed        | `i8` / `i16` / `i32` / `i64`                        | `std::int8_t` / `16` / `32` / `64`                       |
| Float         | `f32` / `f64`                                       | `float` / `double`                                       |
| Pointer-width | `uptr` / `iptr`                                     | `std::uintptr_t` / `std::intptr_t`                       |
| Size/Diff     | `usize` / `isize`                                   | `std::size_t` / `std::ptrdiff_t`                         |
| Char          | `uchar` / `ushort` / `uint` / `ulong` / `ulonglong` | `unsigned char` / `short` / `int` / `long` / `long long` |

```cpp
u32 val = 0xDEADBEEF;
u8  magic[4];
usize len = sizeof(magic);  // 4
uptr addr = rcast<uptr>(&val);
```

---

## Strong Types (stx::off_s, stx::rva_s, stx::va_s)

Type-safe wrappers preventing implicit mixing of semantically distinct numeric domains.

| Type    | Wraps            | Tag          | Domain                   |
| ------- | ---------------- | ------------ | ------------------------ |
| `off_s` | `std::ptrdiff_t` | `offset_tag` | Byte offset              |
| `rva_s` | `u32`            | `rva_tag`    | Relative virtual address |
| `va_s`  | `uptr`           | `va_tag`     | Virtual address          |

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
auto raw    = off.get();     // std::ptrdiff_t = 128
auto as_i64 = off.as<i64>(); // static_cast
i64  direct = off;           // explicit operator i64
```

### Arithmetic (stx::off_s, stx::rva_s, stx::va_s)

| Expression                              | Result type      | Semantics                  |
| --------------------------------------- | ---------------- | -------------------------- |
| `off + 32`                              | `off_s`          | Offset + scalar            |
| `off + other` (same  tag)               | `off_s`          | Offset + offset            |
| `va + off`    (cross-tag)               | `va_s`           | VA + offset = VA           |
| `rva + off`   (cross-tag)               | `rva_s`          | RVA + offset = RVA         |
| `off + rva`   (cross-tag)               | `rva_s`          | Offset + RVA = RVA         |
| `off + va`    (cross-tag)               | `va_s`           | Offset + VA = VA           |
| `32 + off`                              | `std::ptrdiff_t` | Scalar + offset value      |
| `off - other` (same  tag)               | `std::ptrdiff_t` | Difference (loses wrapper) |
| `va - off`    (cross-tag)               | `va_s`           | VA - offset = VA           |
| `rva - off`   (cross-tag)               | `rva_s`          | RVA - offset = RVA         |
| `off - 32`                              | `off_s`          | Offset - scalar            |
| `32 - off`                              | `std::ptrdiff_t` | Scalar - offset value      |
| `va += off` / `va -= off` (cross-tag)   | `va_s&`          | Compound with offset       |
| `rva += off` / `rva -= off` (cross-tag) | `rva_s&`         | Compound with offset       |
| `off += 32` / `off -= 32`               | `off_s&`         | Compound with scalar       |
| `++off` / `--off`                       | `off_s&`         | Pre-increment/decrement    |
| `off++` / `off--`                       | `off_s`          | Post-increment/decrement   |

```cpp
auto a = off + 32;                      // off_s{160}
auto d = off_s{200} - off_s{150};       // ptrdiff_t = 50
++off;                                  // off_s{129}
auto va  = va_s{0x1000} + off_s{8};     // va_s{0x1008}
auto va2 = va_s{0x1000} - off_s{4};     // va_s{0xFFC}
auto rva = rva_s{0x2000} + off_s{8};    // rva_s{0x2008}
va += off_s{16};                        // va_s{0x1018}
```

### Comparison (stx::off_s)

```cpp
off_s{10} <  off_s{20};    // true
off_s{10} == off_s{10};    // true
```

### Usage in APIs (stx::off_s)

Strong types select overloads: `ptr::operator>>` accepts `off_s` directly
(byte-level chase). For byte-level displacement, use `ptr + off_s{n}`:

```cpp
ptr<int> p{buf};
p[2];                        // element index 2 (element-level)
p[2, 1];                     // byte offset 2   (custom stride 1)
auto bp = p + off_s{8};      // byte offset 8 via arithmetic
auto v  = (p + off_s{8}).read<u32>(); // read at byte 8
```

### Why strong types?

| Aspect        | Vanilla C++                                          | stx                                                    |
| ------------- | ---------------------------------------------------- | ------------------------------------------------------ |
| Domain safety | `int off, rva, va` — all interchangeable by accident | `off_s`, `rva_s`, `va_s` — compiler rejects mismatches |
| Arithmetic    | `ptr + (int)offset` — no intent documented           | `ptr + off_s{n}` — self-documenting, byte-level        |
| API boundary  | `read(void* base, int off)` — what unit is `off`?    | `read(address_like, off_s)` — type says "bytes"        |
| Format        | `printf("%td", off)`                                 | `std::print("{}", off)` — works via `operator T`       |

```cpp
// Vanilla C++: what does this function expect?
void read_section(void* base, int offset);

// stx: the type tells the story
void read_section(address_like base, off_s offset);

// Vanilla C++: accidental domain mixing
int file_offset = 0x400;
int rva         = 0x1000;
int va          = 0x140000000;
auto p  = base + file_offset;   // meant bytes?
auto p2 = base + rva;           // but rva is not an offset!

// stx: compiler prevents mixing
off_s file_off {0x400};
rva_s image_rva{0x1000};
va_s  image_va {0x140000000};
auto p  = base + file_off;   // ✓ byte offset
// auto p2 = base + image_rva; // ✗ error: rva_s + ptr is not defined
auto p2 = base + off_s{image_rva}; // ✓ explicit conversion documents intent
```

### Defining your own strong types (stx::newtype)

`newtype<Type, Tag>` is the public building block behind `off_s`/`rva_s`/`va_s`.
It is a distinct type with the same runtime representation as `Type`,
discriminated at compile time by `Tag`. External projects can define their
own strong types without editing this header.

```cpp
template<typename Type, typename Tag> class newtype;
```

A user-defined type is declared with an explicit tag — two lines:

```cpp
struct user_id_tag {};
using user_id_s = stx::newtype<stx::u64, user_id_tag>;
```

- `user_id_s` is **distinct** from both `u64` and from any other `newtype`
  (it never collides with `off_s`/`rva_s`/`va_s` because every tag is unique).
- Conversion from the backing type is `explicit`; the values do not mix with
  raw integrals.
- Full arithmetic, `get()`/`as<T>()`, comparisons and `operator T` are
  available exactly like the built-in types.

#### Byte offsets for external types (stx::offset_s)

The convenient way to make a newtype an **offset-like** type — one that
qualifies for byte-level `ptr<T>[N]`, `ptr + off`, `gap_v`, etc. — is the
`offset_s<Type>` alias. It reuses the built-in `offset_tag`, so it is
mutually convertible with `off_s`/`rva_s`:

```cpp
template<typename Type> using offset_s = newtype<Type, details::offset_tag>;

// 32-bit offsets that are NOT rva's:
using off32_s = stx::offset_s<stx::u32>;

ptr<u8> p{buf};
auto q = p[off32_s{4}];   // byte-level displacement (no * sizeof)
```

#### Custom tag via the is_offset_tag hook

When you need a fully custom tag rather than reusing `offset_tag`, specialize
the public trait `stx::is_offset_tag` to opt a tag into offset-like behaviour
(which also enables the cross-tag offset conversion). By default a tag is NOT
offset-like:

```cpp
template<typename Tag> struct is_offset_tag : std::false_type {};

struct file_tag {};
template<> struct stx::is_offset_tag<file_tag> : std::true_type {};  // opt in

using file_s = stx::newtype<stx::i64, file_tag>;
static_assert(stx::byte_offset<file_s>);   // works with ptr<T>[N]
```

Concepts that honour this hook:

```cpp
template<typename T>
concept byte_offset
    =  requires { typename std::remove_cvref_t<T>::tag_type; }
    and is_offset_tag<typename std::remove_cvref_t<T>::tag_type>::value;
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

| Helper        | Equivalent to            | Grep target |
| ------------- | ------------------------ | ----------- |
| `rcast<T>(v)` | `reinterpret_cast<T>(v)` | `rcast`     |
| `scast<T>(v)` | `static_cast<T>(v)`      | `scast`     |
| `bcast<T>(v)` | `bit_cast<T>(v)`         | `bcast`     |
| `ccast<T>(v)` | `const_cast<T>(v)`       | `ccast`     |
| `dcast<T>(v)` | `dynamic_cast<T>(v)`     | `dcast`     |

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

### Why defer?

| Aspect             | Vanilla C++                              | stx                                       |
| ------------------ | ---------------------------------------- | ----------------------------------------- |
| Early returns      | Manual `free(buf);` before each `return` | `defer` runs destructor automatically     |
| Exceptions         | `catch` block must free                  | Stack unwinding calls destructor          |
| Multiple resources | Nested `try`/`catch` pyramids            | Stacked `defer` in declaration order      |
| Readability        | Cleanup logic mixed with business logic  | Cleanup tied to scope at allocation point |

```cpp
// Vanilla C++: manual cleanup on every path
void* buf  = malloc(1024);
void* buf2 = malloc(2048);
if (!buf || !buf2) { free(buf); free(buf2); return; }
if (cond())        { free(buf); free(buf2); return; }
process(buf, buf2);
free(buf);
free(buf2);

// stx: cleanup tied to scope
void* buf  = malloc(1024);  defer _{[&]{ free(buf);  }};
void* buf2 = malloc(2048);  defer _2{[&]{ free(buf2); }};
if (cond()) return;          // auto-cleaned
process(buf, buf2);          // auto-cleaned on exit
```

---

## `null_t` / `null` (stx::null_t, stx::null)

A null constant distinct from `nullptr`. Does NOT satisfy `address_like`,
preventing accidental API misuse. Supports implicit conversion to any type
constructible from `nullptr_t`.

```cpp
inline constexpr null_t null{};
```

### Key properties (stx::null_t)

| Expression                          | Result                                               |
| ----------------------------------- | ---------------------------------------------------- |
| `null << expr`                      | `null` (discards `expr`, suppresses `[[nodiscard]]`) |
| `static_cast<std::uintptr_t>(null)` | `0`                                                  |
| `static_cast<bool>(null)`           | `false`                                              |
| `static_cast<std::nullptr_t>(null)` | `nullptr`                                            |
| `int* p = null`                     | `nullptr`                                            |
| `std::unique_ptr<T> p = null`       | `nullptr` (via `nullptr_t` ctor)                     |
| `std::shared_ptr<T> p = null`       | `nullptr` (via `nullptr_t` ctor)                     |
| `p == null`                         | `true` (if `p` is null)                              |
| `std::hash<null_t>{}(null)`         | `0`                                                  |
| `std::format("{}", null)`           | `"null"`                                             |

Deleted operators (compile-time error): `null + x`, `null - x`

### Implicit conversions

```cpp
null_t n;

int* raw = n;           // T* overload → nullptr
auto up = std::unique_ptr<int>{n};   // unique_ptr from null
auto sp = std::shared_ptr<int>{n};   // shared_ptr from null
if (n == up) {}         // compare with unique_ptr
if (n == sp) {}         // compare with shared_ptr
```

### Null as discard accumulator (stx::null)

When calling `[[nodiscard]]` functions like `pop()`, chaining via
`null <<` suppresses the warning and discards the value:

```cpp
ptr<u8> p{data};

null << p.pop<u32>()   // read u32, discard, advance
     << p.pop<u16>();  // read u16, discard, advance
```

### Null with `ptr` (stx::null)

```cpp
ptr<int> p{null};    // null pointer
if ( p == null ) {}  // comparison
if ( p ) {}          // bool conversion works too
```

### Why null_t?

| Aspect             | Vanilla C++                          | stx                                      |
| ------------------ | ------------------------------------ | ---------------------------------------- |
| Discard nodiscard  | `(void)pop(); (void)pop();`          | `null << p.pop<u32>() << p.pop<u16>()`   |
| Generic null       | `nullptr` (satisfies `address_like`) | `null` (rejected by `address_like` APIs) |
| Smart pointer init | `unique_ptr<int>{}` or `nullptr`     | `unique_ptr<int>{null}` (implicit)       |
| Pointer check      | `if (p == nullptr)`                  | `if (p == null)` (same but explicit)     |
| Format             | Manual `"null"` string               | `std::print("{}", null)` → `"null"`      |
| Hash               | No standard null hash                | `std::hash<null_t>{}` → `0`              |

```cpp
// Vanilla C++: discard with (void)
(void)p.pop<u32>();  // easy to forget
(void)p.pop<u16>();

// stx: explicit discard chain
null << p.pop<u32>()  // can't forget
     << p.pop<u16>();
```

