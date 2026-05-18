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
- `stx::strong_type` wrappers (e.g., `off_s`, `rav_s`)

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

| Type         | `base_type_t<Type>` |
|--------------|---------------------|
| `u32`        | `u32`               |
| `off_s`      | `ptrdiff_t`         |
| `rva_s`      | `u32`               |
| `enum class Opcode : u32` | `u32` |

---

## Enumerations

### `range_dir`

Direction of iteration.

| Enumerator | Meaning |
|------------|----------|
| `Forward`  | Increasing values |
| `Backward` | Decreasing values |

```cpp
enum class range_dir : u8 {
    Forward,
    Backward
};
```

---

### `range_mode`

Boundary inclusion policy.

| Enumerator   | Meaning |
|--------------|----------|
| `Inclusive`  | End value included |
| `Exclusive`  | End value excluded |

```cpp
enum class range_mode : u8 {
    Inclusive,
    Exclusive
};
```

---

# Public API

## 1. Single-Parameter Range

```cpp
template<rangeable Type>
constexpr auto range(
    Type to,
    range_dir dir,
    range_mode flag = range_mode::Exclusive
) noexcept;
```

Produces range from `0` to `to`.

Default step = `1`.

---

## 2. Full Parameter Range

```cpp
template<rangeable Type>
constexpr auto range(
    Type from,
    Type to,
    base_type_t<Type> step,
    range_dir dir,
    range_mode flag = range_mode::Exclusive
) noexcept;
```

Full control over:

- Start
- End
- Step
- Direction
- Inclusion policy

---

## 3. Simplified Overload

```cpp
template<rangeable Type>
constexpr auto range(
    Type from,
    Type to,
    range_dir dir,
    range_mode flag = range_mode::Exclusive
) noexcept;
```

Defaults step to `1`.

---

## 4. Inclusive Variants (`irange`)

Inclusive convenience wrappers.

```cpp
template<rangeable Type>
constexpr auto irange(
    Type from,
    Type to,
    base_type_t<Type> step,
    range_dir dir
) noexcept;

template<rangeable Type>
constexpr auto irange(
    Type to,
    range_dir dir
) noexcept;
```

Equivalent to `range(..., range_mode::Inclusive)`.

---

## 5. Convenience Overloads (Explicit Type, Auto-Converting Args)

```cpp
// range:
template<rangeable Type> constexpr auto range( auto to ) noexcept;
template<rangeable Type> constexpr auto range( auto from, auto to ) noexcept;
template<rangeable Type> constexpr auto range( auto from, auto to, auto step ) noexcept;
template<rangeable Type> constexpr auto range( auto from, auto to, auto step, range_dir dir, range_mode flag = Exclusive ) noexcept;
template<rangeable Type> constexpr auto range( auto from, auto to, range_dir dir, range_mode flag = Exclusive ) noexcept;
template<rangeable Type> constexpr auto range( auto to, range_dir dir, range_mode flag = Exclusive ) noexcept;

// irange:
template<rangeable Type> constexpr auto irange( auto to ) noexcept;
template<rangeable Type> constexpr auto irange( auto to, range_dir dir ) noexcept;
template<rangeable Type> constexpr auto irange( auto from, auto to, range_dir dir ) noexcept;
template<rangeable Type> constexpr auto irange( auto from, auto to, auto step, range_dir dir ) noexcept;
```

These overloads accept **raw integral** arguments and construct `Type` from them via `Type{value}`.
They only participate when `Type` is a **strong type** or **enum** (`base_type_t<Type> != Type`);
for plain integral `Type` the originals are used instead.

```cpp
// Verbose (original):
for (auto off : range(off_s{0}, off_s{64}, off_s{8}))
    ...

// Ergonomic (convenience):
for (auto off : range<off_s>(0, 64, 8))
    ...
```

### ⚠️ Safety Warning

The call `range<T>(args...)` is equivalent to `range(T{arg1}, T{arg2}, ...)`.
When `T` is an **unsigned** strong type (e.g. `rva_s`, whose base is `u32`),
a negative literal wraps silently:

```cpp
// rva_s is based on u32 (unsigned 32-bit)
range<rva_s>(0, -1, 16)
//  →  range(rva_s{u32{0}}, rva_s{u32{-1}}, 16)
//  →  range(rva_s{0}, rva_s{4294967295}, 16)
```

The `-1` becomes `MAX_U32` — the range spans from `0` to `4 GiB`,
which is almost certainly unintended.

For **signed** strong types (e.g. `off_s`, base `ptrdiff_t`) negative values are safe:

```cpp
range<off_s>(0, -1, 16)   // off_s is signed → -1 stays -1, empty range
```

For **plain integral** `T` (e.g. `u32`), these convenience overloads are
**disabled** (`base_type_t<u32> == u32`). The original `range(Type, Type)`
overloads still apply, with standard C++ implicit narrowing:

```cpp
range<u32>(0, -1)      // compiles (original), -1 wraps to MAX_U32
range<u32>(0, -1, 16)  // does NOT compile — no 3-arg original overload
```

### Available Variants

The convenience family mirrors the original API, supporting all combinations of from/to/step/dir/mode.

| Example | Equivalent |
|---------|-----------|
| `range<off_s>(10)` | `range(off_s{10})` |
| `range<off_s>(0, 64)` | `range(off_s{0}, off_s{64})` |
| `range<off_s>(0, 64, 8)` | `range(off_s{0}, off_s{64}, ptrdiff_t{8}, Forward, Exclusive)` |
| `range<off_s>(0, 64, 8, Backward, Exclusive)` | `range(off_s{0}, off_s{64}, ptrdiff_t{8}, Backward, Exclusive)` |
| `range<off_s>(0, 64, Forward)` | `range(off_s{0}, off_s{64}, ptrdiff_t{1}, Forward, Exclusive)` |
| `range<off_s>(10, Backward)` | `range(off_s{10}, Backward, Exclusive)` |
| `irange<off_s>(10)` | `irange(off_s{10})` |
| `irange<off_s>(0, 64, 8, Forward)` | `irange(off_s{0}, off_s{64}, ptrdiff_t{8}, Forward)` |

---

# Internal Design

## `range_view`

Acts as iterable view.

```cpp
template<rangeable T>
struct range_view
{
    ValueT from;
    ValueT to;
    ValueT step;

    range_dir  dir;
    range_mode mode;

    constexpr auto begin() const noexcept;
    constexpr auto end() const noexcept;
};
```

- Stores underlying base values.
- Produces `range_iter`.
- Uses sentinel termination.

---

## `range_iter`

Iterator implementation using a count-based approach (avoids underflow with unsigned types).

```cpp
template<rangeable Type>
struct range_iter
{
    ValueT cur;
    ValueT step;
    usize  remaining;

    range_dir dir;

    constexpr Type operator*() const noexcept;
    constexpr range_iter& operator++() noexcept;
    constexpr bool operator==(range_sentinel) const noexcept;
};
```

### Behavior

`range` always represents the progression from `from` down/up to `to` by `step`, with one endpoint excluded in exclusive mode. The interval is **always** `[from, to)` for `range` and `[from, to]` for `irange`, regardless of direction — direction only affects traversal order.

| Direction | Mode       | Interval | First Value | Last Value | Example (`from=5, to=0, step=1`) |
|-----------|------------|----------|-------------|------------|----------------------------------|
| Forward   | Exclusive  | `[from, to)` | `from`       | `to - step` | `range(0, 5)` → `{0,1,2,3,4}`   |
| Forward   | Inclusive  | `[from, to]` | `from`       | `to`        | `irange(0, 5)` → `{0,1,2,3,4,5}`|
| Backward  | Exclusive  | `[from, to)` | `from`       | `to + step` | `range(5, 0, Backward)` → `{5,4,3,2,1}` |
| Backward  | Inclusive  | `[from, to]` | `from`       | `to`        | `irange(5, 0, Backward)` → `{5,4,3,2,1,0}`|

Forward and backward exclusive both exclude `to` — the same interval `[from, to)`, just traversed in opposite directions.

#### With `step > 1`

| Example | Full Progression | Exclusive Result |
|---------|-----------------|------------------|
| `range(0, 10, 3, Forward)` | `0, 3, 6, 9` | `{0, 3, 6, 9}` *(to=10 not in progression)* |
| `range(10, 0, 3, Backward)` | `10, 7, 4, 1` | `{10, 7, 4, 1}` *(to=0 not in progression)* |
| `range(30, 0, 3, Backward)` | `30, 27, 24, ..., 3, 0` | `{30, 27, ..., 3}` *(to=0 excluded)* |

#### Internals

| Direction | Step Operation | Termination |
|-----------|---------------|-------------|
| Forward   | `cur += step` | `remaining == 0` |
| Backward  | `cur -= step` | `remaining == 0` |

`remaining` is computed at `begin()`:

| Direction | Mode       | Remaining Count |
|-----------|------------|-----------------|
| Forward   | Exclusive  | `(dist + step - 1) / step` (ceiling division) |
| Forward   | Inclusive  | `dist / step + 1` |
| Backward  | Exclusive  | `(dist + step - 1) / step` (ceiling division) |
| Backward  | Inclusive  | `dist / step + 1` |

Where `dist = to - from` (forward) or `dist = from - to` (backward).

A `step == 0` triggers an assertion failure.

---

# Strong Type & Enum Compatibility

When iterating over strong types:

- Internal arithmetic uses underlying integral.
- Dereference reconstructs strong type.

```cpp
constexpr Type operator*() const noexcept
{
    if constexpr (std::integral<Type>)
        return cur;
    else if constexpr (std::is_enum_v<Type>)
        return static_cast<Type>(cur);
    else
        return Type{cur};
}
```

This preserves domain separation and supports enum iteration.

---

# Example Usage

## Forward Exclusive

```cpp
for (auto i : stx::range(0, 10, 1, stx::range_dir::Forward))
{
    // 0,1,2,3,4,5,6,7,8,9
}
```

---

## Forward Inclusive

```cpp
for (auto i : stx::irange(0, 10, 1, stx::range_dir::Forward))
{
    // 0,1,2,3,4,5,6,7,8,9,10
}
```

---

## Backward Exclusive

```cpp
for (auto i : stx::range(10, 0, 1, stx::range_dir::Backward))
{
    // 10,9,8,7,6,5,4,3,2,1
}
```

---

## Custom Step

```cpp
for (auto i : stx::range(0, 20, 4, stx::range_dir::Forward))
{
    // 0,4,8,12,16
}
```

## Backward with Custom Step

```cpp
for (auto i : stx::range(30, 0, 3, stx::range_dir::Backward))
{
    // 30,27,24,21,18,15,12,9,6,3
}
```

---

## Strong Type Example

```cpp
for (auto off : stx::range(
        stx::off_s{0},
        stx::off_s{64},
        8,
        stx::range_dir::Forward))
{
    // off is off_s
}
```

No implicit conversion occurs between strong types.

## Enum Example

```cpp
enum class Perms : u32 {
    Read    = 1,
    Write   = 2,
    Exec    = 4,
    All     = 7,
};

for (auto p : stx::range(Perms{0}, Perms{8}, stx::range_dir::Forward))
{
    // p is of type Perms: 0, 1, 2, 3, 4, 5, 6, 7
}
```

---

# Design Characteristics

- C++23 constexpr-friendly
- No dynamic allocation
- Sentinel-based iteration
- Strong type safe
- Direction-aware
- Compile-time constrained via concepts
- Header-only
- Zero abstraction overhead

---

# Intended Usage

- Iterating offsets in memory
- Traversing RVA ranges
- Generating aligned index sequences
- Binary structure walking
- Systems-level tooling loops

`range.hpp` provides a domain-safe alternative to ad-hoc integer loops while remaining minimal and constexpr-compatible.

