# Bit & Byte — `bit_extract`, `byte_extract`, etc.

```cpp
#include <lbyte/stx/bit.hpp>
```

Zero-overhead `constexpr` bit and byte manipulation for unsigned integer types.

## Bit Operations

All template parameters are compile-time constants. `Pos` = bit position (0 = LSB). `Len` = number of bits (default 1).

| Function                          | Description                                         |
|-----------------------------------|-----------------------------------------------------|
| `bit_extract<Pos, Len>(v)`        | Extract `Len` bits starting at `Pos`, right-shifted |
| `bit_mask<Pos, Len>(v)`           | Keep only bits `[Pos, Pos+Len)`                     |
| `bit_insert<Pos, Len>(dest, src)` | Insert `src` into bits `[Pos, Pos+Len)` of `dest`   |
| `bit_test<Pos>(v)`                | `true` if bit `Pos` is 1                            |
| `bit_set<Pos>(v)`                 | Return `v` with bit `Pos` set to 1                  |
| `bit_clear<Pos>(v)`               | Return `v` with bit `Pos` set to 0                  |
| `bit_flip<Pos>(v)`                | Return `v` with bit `Pos` flipped                   |

## Byte Operations

`N`, `A`, `B` = byte indices (0 = LSB).

| Function                    | Description                           |
|-----------------------------|---------------------------------------|
| `byte_extract<N>(v)`        | Extract byte `N` as `u8`              |
| `byte_insert<N>(dest, src)` | Replace byte `N` of `dest` with `src` |
| `byte_mask<N>(v)`           | Keep only byte `N`, zero the rest     |
| `byte_swap<A, B>(v)`        | Swap bytes `A` and `B` in-place       |

## Example

```cpp
u32 x = 0x12345678;

bit_extract<4, 4>(x);    // 0x7 (nibble alto del byte bajo)
bit_test<31>(x);         // false
bit_set<0>(x);           // 0x12345679
bit_clear<31>(x);        // 0x12345678

byte_extract<0>(x);      // 0x78 (u8)
byte_extract<3>(x);      // 0x12 (u8)
byte_swap<0, 3>(x);      // x → 0x78345612
```

## Compatibility

- Works with `u8`, `u16`, `u32`, `u64` and any `std::unsigned_integral`
- All functions are `constexpr`
- `static_assert` on out-of-range parameters (compile-time check)

## See Also

- `core.hpp` — type aliases (`u8`, `u32`, `u64`)

