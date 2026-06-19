# range.hpp

## Overview

`range.hpp` implements a lightweight, constexpr-friendly integer range facility for C++23.

It supports:

- Forward and backward iteration
- Inclusive and exclusive bounds
- Custom step values
- Compatibility with integral types and `strong_type`
- Zero dynamic allocation
- Sentinel-based iteration

The design enables domain-safe iteration over:

- Raw integral values
- `stx::strong_type` wrappers (e.g., `off_s`, `rva_s`)
- Enum types

---

## Concepts

### `details::rangeable`

A type is `rangeable` if:

- It is an integral type, or
- It is an enum type, or
- It exposes:
  - `value_type`
  - `.get()` returning an integral

```cpp
template<typename Type>
concept rangeable =
       std::integral<Type>
    or std::is_enum_v<Type>
    or requires(Type t) {
        typename Type::value_type;
        { t.get() } -> std::integral;
    };
```

This allows seamless integration with `strong_type` and enum types.

---

## Base Type Extraction

Used internally to operate on underlying numeric representation.

```cpp
template<typename T>
struct base_type { using type = T; };

template<typename T, typename Tag>
struct base_type<strong_type<T, Tag>> { using type = T; };

template<typename T> requires std::is_enum_v<T>
struct base_type<T> { using type = std::underlying_type_t<T>; };

template<typename T>
using base_type_t = typename base_type<T>::type;
```

For example:

| Type                      | `base_type_t<Type>` |
|---------------------------|---------------------|
| `u32`                     | `u32`               |
| `i64`                     | `i64`               |
| `off_s`                   | `ptrdiff_t`         |
| `rva_s`                   | `u32`               |
| `enum class Opcode : u32` | `u32`               |

---

## Enumerations

### `range_mode`

Boundary inclusion policy.

| Enumerator   | Meaning            |
|--------------|--------------------|
| `Inclusive`  | End value included |
| `Exclusive`  | End value excluded |

```cpp
enum class range_mode : u8 {
    Inclusive,
    Exclusive
};
```

---

# Public API (Explicit Type Only, Auto-Converting Args)

All functions require an explicit `Type` template argument. The direction of iteration is **never** a parameter — it is inferred automatically:

- **1-arg / 2-arg overloads:** direction = `fwd` if `to >= from`, else `bwd`
- **3-arg overloads:** direction = `fwd` if `step >= 0`, else `bwd`
- Step is stored as an absolute magnitude; the sign only selects direction.

| Example                 | Direction | Values             |
|-------------------------|-----------|--------------------|
| `range<int>(5)`         | fwd       | `0, 1, 2, 3, 4`    |
| `range<int>(5, 0)`      | bwd       | `5, 4, 3, 2, 1`    |
| `range<int>(0, 10, 2)`  | fwd       | `0, 2, 4, 6, 8`    |
| `range<int>(10, 0, -2)` | bwd       | `10, 8, 6, 4, 2`   |
| `irange<int>(0, 5)`     | fwd       | `0, 1, 2, 3, 4, 5` |
| `irange<int>(5, 0)`     | bwd       | `5, 4, 3, 2, 1, 0` |

## `range` (Exclusive)

```cpp
template<rangeable Type> constexpr auto range( auto to                       ) noexcept;
template<rangeable Type> constexpr auto range( auto from, auto to            ) noexcept;
template<rangeable Type> constexpr auto range( auto from, auto to, auto step ) noexcept;
```

Step defaults to `+1` in the 1-arg and 2-arg overloads.

The 3-arg overload accepts a **signed** step. A negative step selects backward iteration; the step magnitude is used as the increment.

```cpp
range<int>(0, 10,  2)  // forward , step=2 →  0, 2, 4, 6, 8
range<int>(10, 0, -2)  // backward, step=2 → 10, 8, 6, 4, 2
```

## `irange` (Inclusive)

```cpp
template<rangeable Type> constexpr auto irange( auto to                       ) noexcept;
template<rangeable Type> constexpr auto irange( auto from, auto to            ) noexcept;
template<rangeable Type> constexpr auto irange( auto from, auto to, auto step ) noexcept;
```

Identical to `range` but includes the `to` endpoint:

```cpp
irange<int>(5)           // 0, 1, 2, 3, 4, 5
irange<int>(5, 0)        // 5, 4, 3, 2, 1, 0
irange<int>(0, 10, 3)    // 0, 3, 6, 9
```

---

## Safety: Narrowing Check

Because the overloads accept `auto` arguments and construct `Type{value}`, values that do not fit in the target type are rejected at compile time:

```cpp
range<u8>(255)       // fits
range<u8>(256)       // narrowing error at compile time
range<u8>(0, -1)     // u8{-1} is narrowing
```

This provides a safety net against accidental truncation.

---

## Direction Rules Summary

| Overload                    | Step          | Direction Source    |
|-----------------------------|---------------|---------------------|
| `range<T>(to)`              | `+1`          | `to >= 0     → fwd` |
| `range<T>(from, to)`        | `+1`          | `to >= from  → fwd` |
| `range<T>(from, to, step)`  | user-provided | `step >= 0   → fwd` |
| `irange<T>(to)`             | `+1`          | `to >= 0     → fwd` |
| `irange<T>(from, to)`       | `+1`          | `to >= from  → fwd` |
| `irange<T>(from, to, step)` | user-provided | `step >= 0   → fwd` |

When direction is `bwd`, the iteration starts at `from` and counts **down** toward `to`.

---

## Enum Iteration

Enums are `rangeable` — iterate over enum values without casting:

```cpp
enum class Perms : u32 { Read = 1, Write = 2, Exec = 4, All = 7 };

for (auto p : stx::irange<Perms>(Perms{0}, Perms{7}))
    test_protection(p);              // p is Perms
```

---

## Strong Type Iteration

Domain separation is preserved — the iteration variable retains the strong type:

```cpp
for (auto off : stx::range<stx::off_s>(0, 0x200, 0x28))
    scan_section(data, off);          // off is off_s
```

---

# Internal Design

## `details::dir`

```cpp
namespace details {
    enum class dir : u8 { fwd, bwd };
}
```

Used internally by `range_view` and `range_iter`. Never exposed in the public API.

## `details::range_view`

```cpp
template<rangeable T>
struct range_view
{
    ValueT from;
    ValueT to;
    ValueT step;

    dir        dir_;
    range_mode mode;

    constexpr auto begin() const noexcept;
    constexpr auto end  () const noexcept;
};
```

- Stores underlying base values.
- Produces `range_iter`.
- Uses sentinel termination.

## `details::range_iter`

Iterator implementation using a count-based approach (avoids underflow with unsigned types).

```cpp
template<rangeable Type>
struct range_iter
{
    ValueT cur;
    ValueT step;
    usize  remaining;

    dir dir_;

    constexpr Type operator*() const noexcept;
    constexpr range_iter& operator++() noexcept;
    constexpr bool operator==(range_sentinel) const noexcept;
};
```

### Behavior

The interval is always `[from, to)` for `range` and `[from, to]` for `irange`.

| Direction | Mode       | Interval     | First Value  | Last Value  | Example                               |
|-----------|------------|--------------|--------------|-------------|---------------------------------------|
| fwd       | Exclusive  | `[from, to)` | `from`       | `to - step` | `range<int> (0, 5)` → `{0,1,2,3,4}`   |
| fwd       | Inclusive  | `[from, to]` | `from`       | `to`        | `irange<int>(0, 5)` → `{0,1,2,3,4,5}` |
| bwd       | Exclusive  | `[from, to)` | `from`       | `to + step` | `range<int> (5, 0)` → `{5,4,3,2,1}`   |
| bwd       | Inclusive  | `[from, to]` | `from`       | `to`        | `irange<int>(5, 0)` → `{5,4,3,2,1,0}` |

#### With `step > 1`

| Example                  | Full Progression        | Exclusive Result   |
|--------------------------|-------------------------|--------------------|
| `range<int>( 0, 10,  3)` | `0, 3, 6, 9`            | `{0, 3, 6, 9}`     |
| `range<int>(10,  0, -3)` | `10, 7, 4, 1`           | `{10, 7, 4, 1}`    |
| `range<int>(30,  0, -3)` | `30, 27, 24, ..., 3, 0` | `{30, 27, ..., 3}` |

#### `remaining` computation at `begin()`:

| Direction | Mode       | Remaining Count                       |
|-----------|------------|---------------------------------------|
| fwd       | Exclusive  | `(dist + step - 1) / step` (ceiling)  |
| fwd       | Inclusive  | `dist / step + 1`                     |
| bwd       | Exclusive  | `(dist + step - 1) / step` (ceiling)  |
| bwd       | Inclusive  | `dist / step + 1`                     |

Where `dist = to - from` (fwd) or `dist = from - to` (bwd).

A `step == 0` triggers an assertion failure.

---

# Example Usage

## 1. Scanning Memory Regions

```cpp
for (auto off : stx::range<stx::off_s>(0, 0x200, 0x28))
{
    auto name = reader.read<stx::u64>(off);
    auto vmsz = reader.read<stx::u32>(off + 0x18);
    // off is off_s — type-safe against rva_s/va_s
}
```

## 2. Backward Traversal

```cpp
// Walk backwards using negative step
for (auto off : stx::range<stx::off_s>(1024, 0, -16))
    process(off);   // 1024, 1008, ..., 16

// Or use from>to with implicit step=+1
for (auto off : stx::range<stx::off_s>(1024, 0))
    process(off);   // 1024, 1023, ..., 1
```

## 3. Chunk Processing

```cpp
for (auto off : stx::range<stx::off_s>(1024, 0, 16, /* implicitly exclusive */))
    // Wait — no 4-arg overload. Use:
    //   range<off_s>(1024, 0)  → step=+1, backward
    //   range<off_s>(1024, 0, -16)  → step=16, backward (exclusive of 0)
```

## 4. Compile-Time Sequence Generation

```cpp
constexpr auto sum = [] {
    auto s = 0;
    for (auto i : stx::irange<int>(0, 100))
        s += i;
    return s;
}();
static_assert(sum == 5050);
```

## 5. Enum Iteration

```cpp
enum class Perms : stx::u32 {
    Read    = 1,
    Write   = 2,
    Exec    = 4,
    All     = 7,
};

for (auto p : stx::irange<Perms>(Perms{0}, Perms{7}))
    test_protection(p);
```

## 6. Safety Against Domain Confusion

```cpp
stx::off_s file_off{0x400};
stx::rva_s image_rva{0x1000};

for (auto off : stx::range<stx::off_s>(file_off, file_off + 0x200))
    scan_section(data, off);          // off is off_s
```

---

# Design Characteristics

- C++23 constexpr-friendly
- No dynamic allocation
- Sentinel-based iteration
- Strong type safe
- Direction inferred (no `dir` parameter in public API)
- Compile-time narrowing check on `auto` → `Type` conversion
- Header-only
- Zero abstraction overhead

