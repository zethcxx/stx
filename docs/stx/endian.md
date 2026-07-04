# Endian — `le<T>` / `be<T>`

```cpp
#include <lbyte/stx/endian.hpp>
```

Type-safe wrappers for little-endian and big-endian storage. Guarantee a fixed in-memory byte order regardless of host platform. All names in `namespace lbyte::stx::endian`.

## Concept

```cpp
template<typename T>
concept compatible = byte_swappable<T>; /* std::integral or enum, excluding char/bool */

template<compatible T, order Order>
struct endian_value;

using le = endian_value<T, order::little>;
using be = endian_value<T, order::big>;
```

## Properties

- `sizeof(le<T>) == sizeof(T)`
- Trivially copyable (`memcpy`-safe)
- Standard layout
- Satisfies `binary_readable` — works with `read<endian::le<u32>>(addr)`
- Fully `constexpr` (C++23)

## Example

```cpp
struct header {
    endian::le<u32> signature;
    endian::le<u16> machine;
    endian::be<u64> timestamp;
};

auto h = read<header>(data);
if ( h.signature == 0x4550 )   // works on LE and BE hosts
    process(h);
```

## Type Trait

```cpp
template<typename T>
constexpr bool is_endian_value_v;
```

Returns `true` if `T` is an instantiation of `endian_value<T, Order>` (e.g. `endian::le<u32>` or `endian::be<u16>`).

```cpp
static_assert( is_endian_value_v<endian::le<u32>>);
static_assert(!is_endian_value_v<u32>           );
```

## Methods

| Method                  | Description                                |
|-------------------------|--------------------------------------------|
| `get()`                 | Returns value in native endian             |
| `operator T()`          | Implicit conversion to native endian       |
| `operator=(U)`          | Assign raw value, auto-converts to storage |
| `endian_value(U other)` | Explicit converting ctor from another `endian_value` of different width (e.g. `le<u32>` → `le<u64>`) |
| `data()`                | Pointer to raw storage (for serialization) |
| `swap()`                | Exchange two values                        |

## Operators

**Compound assignment:** `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, `^=`, `<<=`, `>>=`

**Increment/decrement:** `++x`, `x++`, `--x`, `x--`

**Binary arithmetic:** `+`, `-`, `*`, `/`, `%`, `&`, `|`, `^` with `endian_value` or raw `T`

**Shift:** `<<`, `>>`

**Comparison:** `==`, `<=>` with `endian_value` or raw `T` (both sides)

**Unary:** `+`, `-`, `~`

**I/O:** `<<` (ostream), `>>` (istream)

## STL Compatibility

- `std::hash<endian::le<T>>` — same as `hash<T>` of the native value
- `std::formatter<endian::le<T>>` — reuses formatter of `T` (if `<format>` is available)
- `std::swap` — via friend `swap()`

## Why endian_value / le\<T\>?

| Aspect | Vanilla C++ | stx |
|--------|-------------|-----|
| Declaration | `u32 sig;` — no endianness info | `le<u32> sig;` — self-documenting byte order |
| Cross-platform | Manual `#ifdef` / `htole32` / `be32toh` | Same code works on LE and BE hosts |
| Readability | `struct { u32 sig; u16 ver; }` — what endian? | `struct { le<u32> sig; le<u16> ver; }` — explicit |
| Serialization | `hdr.sig = htole32(val); hdr.ver = htole16(val);` | `hdr.sig = val; hdr.ver = val;` — implicit conversion |
| Format | Manual byteswap for display | `std::print("{}", le_val)` — via `formatter<T>` |

```cpp
// Vanilla C++: manual endian handling everywhere
struct Header {
    uint32_t sig;           // little-endian on disk
    uint16_t ver;           // little-endian on disk
};

Header h{};
h.sig = htole32(0x4550);
h.ver = htole16(2);

uint32_t sig = le32toh(h.sig);  // must remember to swap on read
if (sig == 0x4550) { /* ... */ }

// stx: endian-aware types do the work
struct Header {
    endian::le<u32> sig;
    endian::le<u16> ver;
};

Header h{};
h.sig = 0x4550;            // auto-converts to LE storage
h.ver = 2;

if (h.sig == 0x4550)       // auto-converts from LE to native
    process(h);
```

## See Also

- `core.hpp` — type aliases (`u32`, `u64`, etc.)
- `mem.hpp` — `read<T>(addr)` reads `endian::le<T>` correctly from memory
