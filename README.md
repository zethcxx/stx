# STX - Systems Toolbelt for C++23

STX is a low-level, header-only library designed for systems programming, binary analysis, and high-performance data manipulation. It leverages modern C++23 features to provide an expressive API without sacrificing C-style performance.

## Technical Specifications
* **Standard:** C++23 (requires support for `std::to_underlying`, `std::is_constant_evaluated`, and related features).
* **Architecture:** Header-only.
* **Memory Management:** Optimized to eliminate unnecessary zero-initialization in temporary buffers.

---

## Modules and Features

### 1. Memory (mem.hpp)
Features the `dirty_vector<T>` container, a specialized `std::vector` utilizing a custom allocator (`vec_init_allocator`).
* **dirty_vector<T>**: Unlike the standard implementation, this container performs default-initialization rather than value-initialization. This eliminates the CPU overhead of zero-filling memory when data is intended to be overwritten immediately upon allocation.

### 2. File System (fs.hpp)
Provides a streamlined interface for binary I/O operations using `std::istream`.
* **readfs<Type>(file, offset, dir)**: Reads a fixed-size structure from a specific file position.
* **readfs<Type>(file, offset, count, dir)**: Reads multiple elements directly into a `dirty_vector<Type>`.
* **setposfs(file, offset, dir)**: A `seekg` wrapper utilizing the `offset_t` type for improved type safety.

### 3. Core and Types (core.hpp / fn.hpp)
* **Type Aliases**: Fixed-width integer definitions (`u8`, `u32`, `i64`, `usize`, etc.).
* **Casting**: Implementations of `scast`, `rcast`, and `ccast` as readable alternatives to traditional C++ casts.
* **Concepts**: Definition of the `binary_readable` concept to restrict file operations to POD types and memory-contiguous structures.

### 4. Utilities (time.hpp / range.hpp)
* **to_time**: Converts 32-bit Win32/PE timestamps into readable `std::chrono` objects.
* **range**: A sequence generator for simplified `for` loop iterations.

---

## Implementation Example: PE Header Parser

The following example demonstrates how to integrate STX modules to process a Windows executable.

```cpp
#include <stx/stx.hpp>
#include <fstream>

using namespace stx;

auto main() -> int
{
    std::ifstream file { "target.dll", std::ios::binary };
    if ( not file.is_open() ) return EXIT_FAILURE;

    // Read fixed-size headers
    auto dos_header { readfs<IMAGE_DOS_HEADER  >( file ) };
    auto nt_header  { readfs<IMAGE_NT_HEADERS64>( file, offset_t{ dos_header.e_lfanew }) };

    // Calculate aligned offset for the Section Table
    auto sections_offset { offset_t { 0
        + dos_header.e_lfanew
        + sizeof( u32 )
        + sizeof( IMAGE_FILE_HEADER )
        + nt_header.FileHeader.SizeOfOptionalHeader
    }};

    // Bulk read into an optimized buffer (zero-fill omitted)
    auto sections { readfs<IMAGE_SECTION_HEADER>(
        file,
        sections_offset,
        nt_header.FileHeader.NumberOfSections
    )};

    for ( const auto& section : sections ) {
        println("Section: {}", section.get_name());
    }

    return EXIT_SUCCESS;
}
```

---

## Integration via CMake

To include STX as an interface dependency in your project:

```cmake
# CMakeLists.txt
add_subdirectory(extern/stx)
target_link_libraries( <target> PRIVATE stx::stx )
```
