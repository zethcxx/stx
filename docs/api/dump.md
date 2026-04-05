# dump.hpp

## Overview

`dump.hpp` provides a colored hexadecimal memory dump facility for the `stx` namespace under C++23.

It defines:

- A single function for dumping memory regions in hex format.
- ANSI colorized output for improved readability.
- Thread-local buffer for concurrent safety.

This module depends on `mem.hpp` for memory access utilities.

---

## Function: `dump`

```cpp
[[gnu::no_sanitize("address")]]
void dump(address_like auto base, usize size) noexcept;
```

### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `base` | `address_like` | Starting memory address |
| `size` | `usize` | Number of bytes to dump |

### Accepted Address Types

- Raw pointers
- `std::uintptr_t`
- `std::intptr_t`
- `stx::va_s`

---

## Output Format

Each line displays:

```
0xADDRESS: XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX |ASCII..........|
```

### Components

| Component | Width | Description |
|----------|-------|-------------|
| Address | 8/16 chars | Hex address with `0x` prefix |
| Hex bytes | 48 chars | 16 bytes in hex, space-separated |
| ASCII | 16 chars | Printable characters or `.` |

---

## Characteristics

| Property | Value |
|----------|-------|
| Bytes per line | 16 |
| Address width | 8 (32-bit) / 16 (64-bit) |
| ASCII filtering | Non-printable → `.` |
| Colorized address | Yes (ANSI escape code `\x1b[38;5;12m`) |
| Thread safety | Thread-local buffer |
| Alignment | Suppresses ASan for inspection |

---

## Implementation Details

- Uses manual hex encoding for performance.
- Avoids iostream formatting overhead.
- Suppresses ASan (`[[gnu::no_sanitize("address")]]`) to allow inspection of memory regions that may trigger sanitizer warnings.
- Uses `std::println` (C++23) for output.
- Buffer layout: 16 (color) + 2 (`0x`) + 2 (`: `) + 48 (hex) + 16 (ASCII) + 2 (`||`) = 84 padding bytes.

---

## Example Usage

```cpp
#include <lbyte/stx/dump.hpp>

void debug_memory()
{
    stx::va_s base{0x140000000};

    stx::dump(base, 128);

    // Output:
    // 0x140000000: de ad be ef ca fe ba be 12 34 56 78 9a bc de f0 |........4Vx....|
    // 0x140000010: ...
}
```

---

## Safety Model

This header assumes:

- Caller ensures region validity.
- No bounds checking is performed.
- No lifetime validation.
- No race condition protection.

| Responsibility | Owner |
|---------------|-------|
| Memory validity | Caller |
| Alignment correctness | Caller |
| Concurrency safety | Caller (uses thread-local buffer) |

---

## Design Characteristics

- Header-only.
- Zero dynamic allocation.
- Requires C++23 (`std::println`).
- Depends on `mem.hpp` for `address_like` concept and casting utilities.
- Color output requires ANSI-capable terminal.
