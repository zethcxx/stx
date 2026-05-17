# literals.hpp

## Overview

`literals.hpp` provides user-defined literal suffixes for all core STX types. All suffixes are defined in `namespace lbyte::stx::literals`.

Activate with:

```cpp
using namespace lbyte::stx::literals;
```

Or selectively:

```cpp
using lbyte::stx::literals::operator""_u32;
using lbyte::stx::literals::operator""_off_s;
```

---

## Suffix Reference

### Floating-Point

| Suffix | Type | Example |
|--------|------|---------|
| `_f32` | `float` | `3.14_f32`, `42_f32` |
| `_f64` | `double` | `2.718_f64`, `100_f64` |

### Unsigned Integers

| Suffix | Type | Example |
|--------|------|---------|
| `_u8` | `u8` | `12_u8` |
| `_u16` | `u16` | `5000_u16` |
| `_u32` | `u32` | `100000_u32` |
| `_u64` | `u64` | `1'000'000_u64` |

### Signed Integers

| Suffix | Type | Example |
|--------|------|---------|
| `_i8` | `i8` | `-12_i8` |
| `_i16` | `i16` | `-5000_i16` |
| `_i32` | `i32` | `-100000_i32` |
| `_i64` | `i64` | `-1'000'000_i64` |

### Size

| Suffix | Type | Example |
|--------|------|---------|
| `_uz` | `usize` | `42_uz` |

### Strong Types

| Suffix | Type | Example |
|--------|------|---------|
| `_off_s` | `off_s` | `128_off_s`, `-128_off_s` |
| `_rva_s` | `rva_s` | `0x1000_rva_s` |
| `_va_s` | `va_s` | `0xDEAD_BEEF_va_s` |

### Pointer Types

| Suffix | Type | Example |
|--------|------|---------|
| `_ptr` | `ptr<std::byte>` | `0x1000_ptr` |
| `_wptr` | `wptr<std::byte, 1>` | `0x2000_wptr` |

### Size Multiples (powers of 1024)

| Suffix | Type | Value | Example |
|--------|------|-------|---------|
| `_kb` | `usize` | `v * 1024` | `4_kb` = 4096 |
| `_mb` | `usize` | `v * 1024^2` | `2_mb` = 2097152 |
| `_gb` | `usize` | `v * 1024^3` | `1_gb` = 1073741824 |

### Endian Literals

| Suffix | Return Type | Example |
|--------|-------------|---------|
| `_le` | `le<u8>`, `le<u16>`, `le<u32>` or `le<u64>` | `0x1234_le` |
| `_be` | `be<u8>`, `be<u16>`, `be<u32>` or `be<u64>` | `0x5678_be` |

The return type is the smallest endian-wrapped unsigned integer that can hold the value:

| Value Range | Return Type |
|-------------|-------------|
| `0` – `0xFF` | `le<u8>` / `be<u8>` |
| `0x100` – `0xFFFF` | `le<u16>` / `be<u16>` |
| `0x10000` – `0xFFFF'FFFF` | `le<u32>` / `be<u32>` |
| `≥ 0x1'0000'0000` | `le<u64>` / `be<u64>` |

Literals are implemented as template char-pack operators for compile-time value extraction:

```cpp
template<char... Cs>
constexpr auto operator""_le() noexcept;
```

## Usage Notes

Because of pp-number greediness, a literal followed by a dot access requires parentheses:

```cpp
auto x = (4_kb).align_up(...);   // OK
// auto x = 4_kb.align_up(...);  // error: pp-number `4_kb.align_up`
```

---

## Design

- All literal operators are `constexpr`.
- No namespace pollution when unused.
- Each suffix mirrors its corresponding type alias.
- Size literals use 1024-based (kibibyte) convention.
