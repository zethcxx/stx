# Endian — `le<T>` / `be<T>`

```cpp
#include <lbyte/stx/endian.hpp>
```

Type-safe wrappers for little-endian and big-endian storage. Guarantee a fixed in-memory byte order regardless of host platform.

## Concept

```cpp
template<typename T>
concept endian_compatible = byte_swappable<T>; /* std::integral or enum, excluding char/bool */

template<endian_compatible T, byte_order Order>
struct endian_value;

using le = endian_value<T, byte_order::little>;
using be = endian_value<T, byte_order::big>;
```

## Properties

- `sizeof(le<T>) == sizeof(T)`
- Trivially copyable (`memcpy`-safe)
- Standard layout
- Satisfies `binary_readable` — works with `read<le<u32>>(addr)`
- Fully `constexpr` (C++23)

## Example

```cpp
struct header {
    le<u32> signature;
    le<u16> machine;
    be<u64> timestamp;
};

auto h = read<header>(data);
if (h.signature == 0x4550)   // works on LE and BE hosts
    process(h);
```

## Methods

| Method | Description |
|--------|-------------|
| `get()` | Returns value in native endian |
| `operator T()` | Implicit conversion to native endian |
| `operator=` | Assign raw value, auto-converts to storage |
| `data()` | Pointer to raw storage (for serialization) |
| `swap()` | Exchange two values |

## Operators

**Compound assignment:** `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, `^=`, `<<=`, `>>=`

**Increment/decrement:** `++x`, `x++`, `--x`, `x--`

**Binary arithmetic:** `+`, `-`, `*`, `/`, `%`, `&`, `|`, `^` with `endian_value` or raw `T`

**Shift:** `<<`, `>>`

**Comparison:** `==`, `<=>` with `endian_value` or raw `T` (both sides)

**Unary:** `+`, `-`, `~`

**I/O:** `<<` (ostream), `>>` (istream)

## STL Compatibility

- `std::hash<le<T>>` — same as `hash<T>` of the native value
- `std::formatter<le<T>>` — reuses formatter of `T` (if `<format>` is available)
- `std::swap` — via friend `swap()`

## See Also

- `core.hpp` — type aliases (`u32`, `u64`, etc.)
- `mem.hpp` — `read<T>(addr)` reads `le<T>` correctly from memory
