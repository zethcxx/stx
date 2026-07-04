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

| Suffix | Type     | Example                |
|--------|----------|------------------------|
| `_f32` | `float`  | `3.14_f32`, `42_f32`   |
| `_f64` | `double` | `2.718_f64`, `100_f64` |

### Unsigned Integers

| Suffix | Type  | Example         |
|--------|-------|-----------------|
| `_u8`  | `u8`  | `12_u8`         |
| `_u16` | `u16` | `5000_u16`      |
| `_u32` | `u32` | `100000_u32`    |
| `_u64` | `u64` | `1'000'000_u64` |

### Signed Integers

| Suffix | Type  | Example          |
|--------|-------|------------------|
| `_i8`  | `i8`  | `-12_i8`         |
| `_i16` | `i16` | `-5000_i16`      |
| `_i32` | `i32` | `-100000_i32`    |
| `_i64` | `i64` | `-1'000'000_i64` |

### Size

| Suffix | Type    | Example  |
|--------|---------|----------|
| `_uz`  | `usize` | `42_uz`  |
| `_iz`  | `isize` | `-1_iz`  |

### Pointer-Sized Integers

| Suffix  | Type   | Example      |
|---------|--------|--------------|
| `_uptr` | `uptr` | `0xFF_uptr`  |
| `_iptr` | `iptr` | `-1_iptr`    |

### Strong Types

| Suffix   | Type    | Example                   |
|----------|---------|---------------------------|
| `_off_s` | `off_s` | `128_off_s`, `-128_off_s` |
| `_rva_s` | `rva_s` | `0x1000_rva_s`            |
| `_va_s`  | `va_s`  | `0xDEAD_BEEF_va_s`        |

### Pointer Types

| Suffix   | Type          | Example          |
|----------|---------------|------------------|
| `_ptr`   | `ptr<std::byte>` | `0x1000_ptr`  |
| `_ptr8`  | `ptr<u8>`     | `0x1000_ptr8`    |
| `_ptr16` | `ptr<u16>`    | `0x1000_ptr16`   |
| `_ptr32` | `ptr<u32>`    | `0x1000_ptr32`   |
| `_ptr64` | `ptr<u64>`    | `0x1000_ptr64`   |
| `_ptrv`  | `ptr<void>`   | `0x1000_ptrv`   |

### Size Multiples (IEC binary — powers of 1024)

| Suffix | Type    | Value        | Example                  |
|--------|---------|--------------|--------------------------|
| `_kib` | `usize` | `v * 1024`   | `4_kib` = 4096           |
| `_mib` | `usize` | `v * 1024^2` | `2_mib` = 2097152        |
| `_gib` | `usize` | `v * 1024^3` | `1_gib` = 1073741824     |
| `_tib` | `usize` | `v * 1024^4` | `1_tib` ≈ 1.1e12         |
| `_pib` | `usize` | `v * 1024^5` | `1_pib` ≈ 1.13e15        |

### Size Multiples (SI decimal — powers of 1000)

| Suffix | Type    | Value        | Example                   |
|--------|---------|--------------|---------------------------|
| `_kb`  | `usize` | `v * 1000`   | `4_kb` = 4000             |
| `_mb`  | `usize` | `v * 1000^2` | `2_mb` = 2000000          |
| `_gb`  | `usize` | `v * 1000^3` | `1_gb` = 1000000000       |
| `_tb`  | `usize` | `v * 1000^4` | `1_tb` = 1000000000000    |
| `_pb`  | `usize` | `v * 1000^5` | `1_pb` = 1000000000000000 |

### Endian Literals

| Suffix | Return Type                                 | Example     |
|--------|---------------------------------------------|-------------|
| `_le`  | `le<u8>`, `le<u16>`, `le<u32>` or `le<u64>` | `0x1234_le` |
| `_be`  | `be<u8>`, `be<u16>`, `be<u32>` or `be<u64>` | `0x5678_be` |

The return type is the smallest endian-wrapped unsigned integer that can hold the value:

| Value Range               | Return Type           |
|---------------------------|-----------------------|
| `0` – `0xFF`              | `le<u8>` / `be<u8>`   |
| `0x100` – `0xFFFF`        | `le<u16>` / `be<u16>` |
| `0x10000` – `0xFFFF'FFFF` | `le<u32>` / `be<u32>` |
| `≥ 0x1'0000'0000`         | `le<u64>` / `be<u64>` |

Literals are implemented as template char-pack operators for compile-time value extraction:

```cpp
template<char... Cs>
constexpr auto operator""_le() noexcept;
```

### Fixed-Width Endian

| Suffix   | Return Type | Example       |
|----------|-------------|---------------|
| `_le16`  | `le<u16>`   | `0x1234_le16` |
| `_le32`  | `le<u32>`   | `0x1234_le32` |
| `_le64`  | `le<u64>`   | `0x1234_le64` |
| `_be16`  | `be<u16>`   | `0x5678_be16` |
| `_be32`  | `be<u32>`   | `0x5678_be32` |
| `_be64`  | `be<u64>`   | `0x5678_be64` |

Unlike `_le`/`_be`, these always return the specified width regardless of the value.

### String → Integer (ASCII pack)

| Suffix     | Return Type                         | Example               |
|------------|-------------------------------------|-----------------------|
| `_istr`    | `u8`, `u16`, `u32` or `u64` (auto) | `"MZ"_istr` → `u16`    |
| `_istr_be` | `u8`, `u16`, `u32` or `u64` (auto) | `"MZ"_istr_be` → `u16` |

Packs ASCII characters into an unsigned integer. The size is deduced from the string length (up to 8 bytes). `_istr` packs in little-endian order, `_istr_be` in big-endian.

### String → Byte Block

| Suffix  | Return Type           | Example                    |
|---------|-----------------------|----------------------------|
| `_vstr` | `byte_block<N>`       | `"PE"_vstr` → `{'P','E'}`  |

Produces a `byte_block<N>` from a string literal.

## Usage Notes

Because of pp-number greediness, a literal followed by a dot access requires parentheses:

```cpp
auto x = (4_kib).align_up(...);   // OK
// auto x = 4_kib.align_up(...);  // error: pp-number `4_kib.align_up`
```

---

## Design

- All literal operators are `constexpr`.
- No namespace pollution when unused.
- Each suffix mirrors its corresponding type alias.
- Size literals include both IEC binary (`_kib`/`_mib`/`_gib`/`_tib`/`_pib`, 1024-base) and SI decimal (`_kb`/`_mb`/`_gb`/`_tb`/`_pb`, 1000-base) prefixes.

