# Bit & Byte — `bit_extract`, `byte_extract`, etc.

```cpp
#include <lbyte/stx/bit.hpp>
```

Zero-overhead `constexpr` bit and byte manipulation for unsigned integer types.

## Bit Operations

All template parameters are compile-time constants. `Pos` = bit position (0 = LSB). `Len` = number of bits (default 1).

| Function | Description |
|----------|-------------|
| `bit_extract<Pos, Len>(v)` | Extrae `Len` bits empezando en `Pos`, desplazado a derecha |
| `bit_mask<Pos, Len>(v)` | Conserva solo los bits `[Pos, Pos+Len)` |
| `bit_insert<Pos, Len>(dest, src)` | Inserta `src` en los bits `[Pos, Pos+Len)` de `dest` |
| `bit_test<Pos>(v)` | `true` si el bit `Pos` está a 1 |
| `bit_set<Pos>(v)` | Devuelve `v` con el bit `Pos` a 1 |
| `bit_clear<Pos>(v)` | Devuelve `v` con el bit `Pos` a 0 |
| `bit_flip<Pos>(v)` | Devuelve `v` con el bit `Pos` invertido |

## Byte Operations

`N`, `A`, `B` = byte indices (0 = LSB).

| Function | Description |
|----------|-------------|
| `byte_extract<N>(v)` | Extrae el byte `N` como `u8` |
| `byte_insert<N>(dest, src)` | Reemplaza el byte `N` de `dest` con `src` |
| `byte_mask<N>(v)` | Conserva solo el byte `N` |
| `byte_swap<A, B>(v)` | Intercambia los bytes `A` y `B` in-place |

## Example

```cpp
u32 x = 0x12345678;

bit_extract<4, 4>(x);   // 0x7 (nibble alto del byte bajo)
bit_test<31>(x);         // false
bit_set<0>(x);           // 0x12345679
bit_clear<31>(x);        // 0x12345678

byte_extract<0>(x);      // 0x78 (u8)
byte_extract<3>(x);      // 0x12 (u8)
byte_swap<0, 3>(x);      // x → 0x78345612
```

## Compatibility

- Funciona con `u8`, `u16`, `u32`, `u64` y cualquier `std::unsigned_integral`
- Todas las funciones son `constexpr`
- `static_assert` en parámetros fuera de rango (compile-time check)

## See Also

- `core.hpp` — type aliases (`u8`, `u32`, `u64`)
