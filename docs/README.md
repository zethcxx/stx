# STX Documentation

> API Reference for STX - Systems Toolbelt for C++23

---

## Module Index

| Module | Header | Description |
|--------|--------|-------------|
| Core | `core.hpp` | Fundamental types, strong types, concepts |
| Memory | `mem.hpp` | Low-level memory access primitives |
| Function | `fn.hpp` | Function pointer abstractions |
| File | `fs.hpp` | Binary file stream utilities |
| Time | `time.hpp` | UNIX time and stopwatch utilities |
| Range | `range.hpp` | Integer range iteration |
| Dump | `dump.hpp` | Hexadecimal memory dump |

---

## Key Types

### Strong Types

| Type | Underlying | Purpose |
|------|------------|---------|
| `off_s` | `usize` | Strong offset (replaces legacy `off_t`) |
| `rva_s` | `u32` | Relative virtual address |
| `va_s` | `uptr` | Absolute virtual address |

### Fundamental Aliases

Width-defined integers: `u8`, `u16`, `u32`, `u64`, `i8`, `i16`, `i32`, `i64`
Pointer-sized: `uptr`, `iptr`, `usize`, `isize`
Floating: `f32`, `f64`

---

## Concepts

| Concept | Description |
|--------|-------------|
| `address_like` | Types usable as memory addresses |
| `binary_readable` | Types safe for raw binary deserialization |

---

## Quick Start

```cpp
#include <lbyte/stx.hpp>

using namespace lbyte::stx;

auto main() -> int
{
    off_s offset{128};

    for (auto i : range(10, range_dir::Forward))
    {
        // iteration
    }

    return EXIT_SUCCESS;
}
```

---

## Additional Resources

- [Main README](../README.md) - Project overview and integration guides
- [Core API](./api/core.md) - Detailed core documentation