# mem.hpp

## Overview

`mem.hpp` provides low-level memory access primitives and bit/align utilities for the `stx` namespace under C++23.

It defines:

- Copy-based typed memory reads and writes (well-defined behavior).
- Raw pointer-based reads and writes (direct dereference).
- Casting helpers (`rcast`, `scast`, `bcast`).
- Alignment utilities for integral and strong types.
- A colored hexadecimal memory dump facility.

The module is intended for controlled low-level environments such as:

- Binary parsing
- Manual mapping
- Memory inspection
- Instrumentation
- Runtime patching

All functions assume the caller guarantees memory validity, lifetime correctness, and access permissions.

---

## Force Inline Macro

| Macro | Meaning |
|--------|----------|
| `STX_FORCE_INLINE` | Expands to `[[gnu::always_inline]] inline` on GCC/Clang, otherwise `inline` |

Used to ensure minimal abstraction overhead in critical paths.

---

# Typed Memory Access

## 1. Copy-Based Read

```cpp
template<class Type, address_like Addr>
[[nodiscard]] STX_FORCE_INLINE
Type read(Addr base, offset_t off = offset_t{0}) noexcept;
```

### Semantics

- Uses `std::memcpy`.
- Behavior is defined under the C++ object model.
- Safe for unaligned memory.
- Safe under strict aliasing rules.
- Requires `std::is_trivially_copyable_v<Type>`.

### When to Use

| Scenario | Recommended |
|----------|-------------|
| Parsing binary structures | Yes |
| Packed / unaligned memory | Yes |
| Portable code | Yes |
| Memory-mapped files | Yes |

---

## 2. Raw Read (Direct Dereference)

```cpp
template<class Type>
[[nodiscard]] STX_FORCE_INLINE
Type read_raw(address_like auto base,
              offset_t off = offset_t{0}) noexcept;
```

### Semantics

- Direct pointer dereference.
- No `memcpy`.
- Requires aligned memory.
- May violate strict aliasing if misused.
- Requires `std::is_trivially_copyable_v<Type>`.

### When to Use

| Scenario | Recommended |
|----------|-------------|
| Known-aligned internal buffers | Yes |
| Performance-critical tight loops | Yes |
| Arbitrary mapped memory | No |

---

# Typed Memory Write

## 3. Copy-Based Write

```cpp
template<class Type, address_like Addr>
STX_FORCE_INLINE
void write(Addr base, offset_t off, Type value) noexcept;
```

### Semantics

- Uses `std::memcpy`.
- Defined behavior.
- Safe for unaligned memory.
- Requires trivially copyable type.

---

## 4. Raw Write (Direct Dereference)

```cpp
template<class Type>
STX_FORCE_INLINE
void write_raw(address_like auto base,
               offset_t off,
               Type value) noexcept;
```

### Semantics

- Direct assignment through cast pointer.
- Requires valid alignment.
- No aliasing guarantees.
- Suitable for controlled memory regions.

---

# Casting Utilities

## reinterpret_cast Wrapper

```cpp
template<class Type>
STX_FORCE_INLINE
Type rcast(auto value) noexcept;
```

Thin wrapper over `reinterpret_cast`.

Used to centralize casting style and improve readability in low-level code.

---

## static_cast Wrapper

```cpp
template<class Type>
STX_FORCE_INLINE
constexpr Type scast(auto value) noexcept;
```

Thin wrapper over `static_cast`.

Primarily used for:

- Narrowing control
- Integral conversions
- Enum conversions

---

## Bit Cast

```cpp
template<typename To, typename From>
[[nodiscard]] STX_FORCE_INLINE
constexpr To bcast(const From& from) noexcept;
```

Wrapper over `std::bit_cast`.

### Requirements

| Constraint | Description |
|------------|-------------|
| `sizeof(To) == sizeof(From)` | Required by `std::bit_cast` |
| Trivially copyable types | Required |

Used for safe reinterpretation of object representation.

---

# Alignment Utilities

## 1. Integral Alignment

```cpp
template<std::unsigned_integral T>
constexpr T align_up(T value, T alignment) noexcept;

template<std::unsigned_integral T>
constexpr T align_down(T value, T alignment) noexcept;
```

### Behavior

| Function | Formula |
|----------|---------|
| `align_up` | `(value + alignment - 1) & ~(alignment - 1)` |
| `align_down` | `value & ~(alignment - 1)` |

### Requirements

- `alignment` must be power-of-two.
- Undefined behavior otherwise.

---

## 2. Strong Type Alignment

```cpp
template<typename T, typename Tag>
constexpr auto align_up(details::strong_type<T, Tag> st, T alignment) noexcept;

template<typename T, typename Tag>
constexpr auto align_down(details::strong_type<T, Tag> st, T alignment) noexcept;
```

Preserves strong type domain.

| Input | Output |
|--------|--------|
| `offset_t` | `offset_t` |
| `rva_t` | `rva_t` |
| `va_t` | `va_t` |

No domain leakage occurs.

---

# Hex Dump Facility

## dump

```cpp
[[gnu::no_sanitize("address")]]
void dump(address_like auto base, usize size) noexcept;
```

Produces a colored hexadecimal dump of memory.

### Output Format

Per line:

```
0xADDRESS: XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX |ASCII..........|
```

### Characteristics

| Property | Value |
|----------|--------|
| Bytes per line | 16 |
| Address width | 8 (32-bit) / 16 (64-bit) |
| ASCII filtering | Non-printable → `.` |
| Colorized address | Yes (ANSI escape) |
| Thread safety | Thread-local buffer |

### Implementation Notes

- Uses manual hex encoding.
- Avoids iostream formatting overhead.
- Suppresses ASan for direct memory inspection.
- Uses `std::println` (C++23).

---

# Safety Model

This header assumes:

- Caller ensures region validity.
- No bounds checking is performed.
- No lifetime validation.
- No race condition protection.

| Responsibility | Owner |
|---------------|-------|
| Memory validity | Caller |
| Alignment correctness | Caller |
| Concurrency safety | Caller |

---

# Design Characteristics

- Header-only.
- Zero dynamic allocation.
- constexpr-friendly where possible.
- Explicit separation between defined and raw memory semantics.
- Strong type compatibility (`offset_t`, `va_t`, `rva_t`).
- C++23 compliant.

---

# Example Usage

---

## Safe Read

```cpp
struct header
{
    stx::u32 magic;
    stx::u16 version;
    stx::u16 flags;
};

stx::va_t base{0x140000000};

header h = stx::read<header>(base, stx::offset_t{0x100});
```

---

## Raw Read

```cpp
stx::u32 value =
    stx::read_raw<stx::u32>(base, stx::offset_t{0x200});
```

---

## Write

```cpp
stx::write<stx::u32>(base, stx::offset_t{0x300}, 0xDEADBEEF);
```

---

## Alignment

```cpp
stx::offset_t off{123};
auto aligned = stx::align_up(off, 16);
```

---

## Hex Dump

```cpp
stx::dump(base, 128);
```

---

# Intended Usage Domain

- PE/ELF/Mach-O loaders
- Memory inspection tools
- Runtime patching frameworks
- Red team internal tooling
- Systems-level diagnostics

`mem.hpp` provides a controlled abstraction layer for raw memory manipulation while preserving C++23 compile-time guarantees where applicable.
