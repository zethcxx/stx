# STX Documentation

> API Reference for STX - Systems Toolbelt for C++23

---

## Module Index

| Module | Header | Description |
|--------|--------|-------------|
| Core | `core.hpp` | Fundamental types, strong types, concepts |
| Memory | `mem.hpp` | Low-level memory access, `ptr<T>` |
| Function | `fn.hpp` | Function pointer abstractions |
| File | `io.hpp` | Binary file stream utilities |
| Time | `time.hpp` | UNIX time and stopwatch utilities |
| Range | `range.hpp` | Integer range iteration |
| Cycle | `cycle.hpp` | Infinite / bounded range cycling ([docs](./stx/cycle.md)) |
| Literals | `literals.hpp` | Literal suffixes for all core types ([docs](./api/literals.md)) |
| String   | `ct.hpp`       | Compile-time string transforms ([docs](./stx/ct.md)) |

---

## Key Types

### Strong Types

| Type | Underlying | Purpose |
|------|------------|---------|
| `off_s` | `ptrdiff_t` | Strong offset (replaces legacy `off_t`) |
| `rva_s` | `u32` | Relative virtual address |
| `va_s` | `uptr` | Absolute virtual address |

### Pointer Wrappers

| Type | Description |
|------|-------------|
| `ptr<T>` | Typed non-owning pointer: `->`, `raw()` → `T*`, `addr()` → `uptr`, `read<T>()`, `write<T>()`, `call<Sig>()` |
| `ptr<T>` | Typed pointer with walk/chase: `walk()`, `operator>>`, `read/write<T>`, `align_up/down` |

### Fundamental Aliases

Width-defined integers: `u8`, `u16`, `u32`, `u64`, `i8`, `i16`, `i32`, `i64`
Pointer-sized: `uptr`, `iptr`, `usize`, `isize`
Floating: `f32`, `f64`

---

## Concepts

| Concept | Description |
|--------|-------------|
| `address_like` | Types usable as memory addresses |
| `binary_readable` | Types safe for raw binary deserialization (excluding pointers) |
| `byte_swappable` | Types suitable for byte swapping (integral + enum, excluding char types) |

---

## Quick Start

```cpp
#include <lbyte/stx.hpp>

using namespace lbyte::stx;

auto main() -> int
{
    off_s offset{128};

    for (auto i : range<int>(10))
    {
        // iteration
    }

    return EXIT_SUCCESS;
}
```

---

## Additional Resources

- [Main README](../README.md) - Project overview and integration guides
 - [Core API](./stx/core.md) - Detailed core documentation
- [Memory API](./stx/mem.md) - Memory access, ptr
- [Function API](./stx/fn.md) - Function pointer invocations
- [File API](./stx/io.md) - Binary file stream utilities
- [Time API](./stx/time.md) - UNIX time and stopwatch
- [Range API](./stx/range.md) - Integer range iteration
- [Cycle API](./stx/cycle.md) - Infinite / bounded range cycling
- [Literals API](./stx/literals.md) - Literal suffixes for core types
- [String API](./stx/ct.md) - Compile-time string literals
