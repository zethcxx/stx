# STX - Systems Toolbelt for C++23
> Disclaimer: This project is intended for personal use and experimentation. Users are free to fork or modify it, but all usage is at their own risk. The author provides no guarantees regarding functionality, security, or safety.

**Version:** 2.0.0

STX is a header-only C++23 library providing a rich set of low-level abstractions and utilities for systems programming, binary analysis, runtime instrumentation, and scripting at the OS/hardware interface. It emphasizes type safety, zero-overhead abstractions, and modern C++ idioms to enhance productivity in reverse engineering, red teaming, and tooling for binary formats.

---

## Technical Specifications

| Feature                  | Description |
|---------------------------|------------|
| Language Standard         | C++23 |
| Header-only               | Yes |
| C++ Modules               | Yes |
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
// also <lbyte/stx/core.hpp>
#include <lbyte/stx.hpp> // or import lbyte.stx if using modules
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

>[!IMPORTANT]
> C++ Modules Requirements: Using modules requires CMake 3.28+ and a compatible compiler (Clang 16+, GCC 14+, or MSVC 19.34+).

### Standard

```cmake
add_subdirectory(extern/stx)

# Option B: Enable C++23 Modules
# set(STX_USE_MODULES ON CACHE BOOL "" FORCE)
# add_subdirectory(extern/stx)

target_link_libraries(<target> PRIVATE lbyte::stx)
```

### FetchContent

```cmake
include(FetchContent)

FetchContent_Declare(
    stx
    GIT_REPOSITORY https://github.com/zethcxx/stx.git
    GIT_TAG        v2.0.0
)

# To use modules with FetchContent:
# set(STX_USE_MODULES ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(stx)

target_link_libraries(${PROJECT_NAME} PRIVATE lbyte::stx)
```


### Xmake
Since this library is not yet available on **xrepo**, you can use this quick workaround in your `xmake.lua`:

```lua
package("zethcxx.stx")
    set_kind("library", { headeronly = true })
    set_urls("https://github.com/zethcxx/stx.git")

    add_versions( "v1.0.0", "v1.0.0" ) -- Or hash
    add_versions( "v2.0.0", "v2.0.0" )

    add_configs( "use_modules", {
        builtin = false,
        default = false,
        type    = "boolean",
        description = "Use C++ Modules"
    })

    on_install( function( package )
        local configs = {}

        if package:config( "use_modules" ) then
            configs.use_modules = true
        end

        import("package.tools.xmake").install( package, configs )
    end)

    on_load( function( package )
        package:add("includedirs", "include")

        if package:config("use_modules") then
            package:add("cxxmodules", "modules/stx/*.cppm")
        end
    end)

    on_test( function (package)
        package:check_cxxsnippets({ test = "import lbyte.stx; int main() { return 0; }"}, { configs = { languages = "c++23" }})
    end)
package_end()

add_requires( "zethcxx.stx v2.0.0"
    -- , { configs = { use_modules = true }} -- if modules is required
)

-- using this in the target:
-- add_packages("zethcxx.stx")
-- set_policy("build.c++.modules", true)
```

## Troubleshooting

### Missing Clang Resource Headers
When using C++ Modules with Clang, you might encounter errors like `fatal error: 'stddef.h' file not found` or missing `__stddef_max_align_t.h`. This happens when the compiler cannot locate its internal resource directory.

#### Quick Fix (Shell)
Export the resource path to your environment:

```bash
# For Clang 21 (Adjust version if necessary)
export CPLUS_INCLUDE_PATH="/usr/lib/clang/21/include/":$CPLUS_INCLUDE_PATH
```

---

## Design Principles

- Header-only, zero-runtime overhead abstractions
- Strong typing for offsets, addresses, and function signatures
- Explicit memory and file safety, no hidden side effects
- C++23 constexpr-friendly, usable in compile-time contexts
- Focused on low-level tooling, scripting, reverse engineering, and red-team operations
