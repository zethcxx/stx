# STX - Systems Toolbelt for C++23
> Disclaimer: This project is intended for personal use and experimentation. Users are free to fork or modify it, but all usage is at their own risk. The author provides no guarantees regarding functionality, security, or safety.

**Version:** 1.0.0

STX is a header-only C++23 library providing a rich set of low-level abstractions and utilities for systems programming, binary analysis, runtime instrumentation, and scripting at the OS/hardware interface. It emphasizes type safety, zero-overhead abstractions, and modern C++ idioms to enhance productivity in reverse engineering, red teaming, and tooling for binary formats.

---

## Technical Specifications

| Feature                  | Description |
|---------------------------|------------|
| Language Standard         | C++23 |
| Header-only               | Yes |
| Dependencies              | Standard library only |
| Memory Management         | Optimized `dirty_vector` avoids unnecessary zero-initialization |
| Target Domains            | Binary analysis, runtime patching, low-level tooling, scripting |

---

## Core Modules

### 1. Memory Utilities (`mem.hpp`)

Provides safe, low-level memory access and alignment primitives, along with a colored hexadecimal dump.

| Component | Description |
|-----------|-------------|
| `read<Type>(addr, offset)` | Copy-based memory read; safe for unaligned memory |
| `read_raw<Type>(addr, offset)` | Direct dereference; requires alignment |
| `write<Type>(addr, offset, value)` | Copy-based write |
| `write_raw<Type>(addr, offset, value)` | Direct write; high-performance, alignment-sensitive |
| `align_up` / `align_down` | Aligns integral or strong types to power-of-two boundaries |
| `dump(addr, size)` | Produces ANSI-colored memory hex dump for inspection |

Intended use:

- Runtime patching
- Memory inspection tools
- Loader implementations
- PE/ELF/Mach-O parsers
- Reverse engineering utilities

---

### 2. File System Utilities (`fs.hpp`)

Provides type-safe, binary-oriented file operations over `std::istream`.

| Function | Description |
|----------|-------------|
| `readfs<Type>(file, offset, dir)` | Reads a single object from a stream |
| `readfs<Type>(file, offset, count, dir)` | Reads multiple objects into `dirty_vector<Type>` |
| `readfs<Type, Size>(file, offset, dir)` | Reads a fixed-size array |
| `setposfs(file, offset, dir)` | Strongly-typed stream positioning |
| `skipfs<Type>(file, offset)` | Moves stream forward by `offset` elements |
| `last_read_ok(file)` | Checks stream state |

Characteristics:

- Enforces type and offset correctness
- Explicit stream state management
- Minimal runtime overhead
- Binary parsing and structured file extraction

---

### 3. Function Abstractions (`fn.hpp`)

Provides strongly-typed wrappers around arbitrary memory addresses for callable functions.

| Component | Description |
|-----------|-------------|
| `caller_t<Sig>` | Wraps a function pointer with compile-time signature enforcement |
| `operator()` | Invokes the function directly |
| `operator bool` | Checks for null |
| `caller<Sig>(addr)` | Factory to produce a `caller_t` from any `address_like` value |

Intended use:

- Manual symbol resolution
- Dynamic loader hooks
- Low-level instrumentation
- JIT function invocation

---

### 4. Time Utilities (`time.hpp`)

Provides portable UNIX time conversions and high-resolution stopwatch functionality.

| Component | Description |
|-----------|-------------|
| `from_unix_seconds(u64)` | Converts seconds since epoch to `time_point` |
| `from_unix_millis(u64)` | Converts milliseconds since epoch to `time_point` |
| `to_unix_seconds(tp)` / `to_unix_millis(tp)` | Converts `time_point` to integer timestamps |
| `unix_seconds_now()` / `unix_millis_now()` | Current UNIX time |
| `stop_watch` | Measures elapsed time in various resolutions |

---

### 5. Ranges (`range.hpp`)

Provides domain-safe, constexpr-friendly integer and strong-type ranges for iteration.

| Feature | Description |
|---------|------------|
| Forward / backward iteration | Controlled by `range_dir` |
| Inclusive / exclusive bounds | Controlled by `range_mode` |
| Step values | Customizable for loops |
| Strong type support | Iterates over `offset_t`, `rva_t`, etc., preserving type safety |

---

## Example Usage: Binary Section Reader

This demonstration shows STX utilities for parsing a PE file section table.

```cpp
#include <stx/stx.hpp>
#include <fstream>

using namespace stx;

auto main() -> int
{
    std::ifstream file { "target.dll", std::ios::binary };
    if ( not file.is_open() ) return EXIT_FAILURE;

    // Read DOS and NT headers
    auto dos  = readfs<IMAGE_DOS_HEADER  >(file);
    auto nt   = readfs<IMAGE_NT_HEADERS64>(file, offset_t{dos.e_lfanew});

    // Calculate Section Table offset
    auto sections_offset = offset_t{
        dos.e_lfanew
        + sizeof(u32)
        + sizeof(IMAGE_FILE_HEADER)
        + nt.FileHeader.SizeOfOptionalHeader
    };

    // Bulk read sections into optimized buffer
    auto sections = readfs<IMAGE_SECTION_HEADER>(
        file,
        sections_offset,
        nt.FileHeader.NumberOfSections
    );

    for (const auto& sec : sections) {
        println("Section: {}", sec.get_name());
    }

    return EXIT_SUCCESS;
}
```

Demonstrates:

- Type-safe file reading
- `dirty_vector` bulk allocation
- Offset arithmetic with `offset_t`
- Strong-type safe iteration

---

## Integration with CMake

### Standard

```cmake
add_subdirectory(extern/stx)
target_link_libraries(<target> PRIVATE stx::stx)
```

### FetchContent

```cmake
include(FetchContent)

FetchContent_Declare(
    stx
    GIT_REPOSITORY https://github.com/zethcxx/stx.git
    GIT_TAG        v1.0.0
)

FetchContent_MakeAvailable(stx)
target_link_libraries(${PROJECT_NAME} PRIVATE stx::stx)
```

---

## Design Principles

- Header-only, zero-runtime overhead abstractions
- Strong typing for offsets, addresses, and function signatures
- Explicit memory and file safety, no hidden side effects
- C++23 constexpr-friendly, usable in compile-time contexts
- Focused on low-level tooling, scripting, reverse engineering, and red-team operations
