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
| C++ Modules               | Yes |
| Dependencies              | Standard library only |
| Memory Management         | Optimized `fs::dirty_vector` avoids unnecessary zero-initialization |
| Target Domains            | Binary analysis, runtime patching, low-level tooling, scripting |

---

## Core Modules

### 1. Core Types (`core.hpp`)

Foundation for the entire library.

| Component | Description |
|-----------|-------------|
| `off_s` | **Strong offset type** (replaces legacy `off_t`) |
| `rva_s` | Strong RVA |
| `va_s` | Strong virtual address |
| `address_like` | Concept for address types |
| `binary_readable` | Concept for binary-safe types |

---

### 2. Memory Utilities (`mem.hpp`)

Provides safe, low-level memory access, alignment primitives, and typed pointer wrappers.

| Component | Description |
|-----------|-------------|
| `read<Type>(addr, offset)` | Copy-based memory read; safe for unaligned memory |
| `read_raw<Type>(addr, offset)` | Direct dereference; requires alignment |
| `write<Type>(addr, offset, value)` | Copy-based write |
| `write_raw<Type>(addr, offset, value)` | Direct write; high-performance, alignment-sensitive |
| `ptr<T>` | Typed non-owning pointer: `->`, `raw()` → `T*`, `uptr()` → `uptr`, `read<T>()`, `write<T>()`, `call<Sig>()` |
| `ptr<T>` | Typed pointer with walk/chase: `walk()`, `operator>>`, `read/write<T>`, `align_up/down` |
| `mem::align_up` / `mem::align_down` | Aligns integral or strong types to power-of-two boundaries |
| `mem::gap_v` / `mem::gap_align_v` | Compile-time size calculators |

Intended use:

- Runtime patching
- Memory inspection tools
- Loader implementations
- PE/ELF/Mach-O parsers
- Reverse engineering utilities

---

### 3. File System Utilities (`fs.hpp`)

Provides type-safe, binary-oriented file operations over `std::istream`.

| Function | Description |
|----------|-------------|
| `readfs<Type>(file, offset, dir)` | Reads a single object from a stream |
| `readfs<Type>(file, offset, count, dir)` | Reads multiple objects into `dirty_vector<Type>` |
| `readfs<Type, Size>(file, offset, dir)` | Reads a fixed-size array |
| `setposfs(file, offset, dir)` | Strongly-typed stream positioning |
| `skipfs(file, offset)` | Moves stream forward by `offset` elements |

Characteristics:

- Enforces type and offset correctness
- Explicit stream state management
- Minimal runtime overhead
- Binary parsing and structured file extraction

---

### 4. Function Abstractions (`fn.hpp`)

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

### 5. Time Utilities (`time.hpp`)

Provides portable UNIX time conversions and high-resolution stopwatch functionality.

| Component | Description |
|-----------|-------------|
| `time::from_unix<Dur>(u64)` | Creates `time_point` from UNIX timestamp |
| `time::to_unix<Dur>(tp)` | Extracts integer UNIX timestamp from `time_point` |
| `time::now()` / `time::now_ms()` / `time::now_ns()` | Current UNIX time |
| `time::stopwatch` | Measures elapsed time (monotonic), with `lap()` support |
| `time::from_filetime` / `to_filetime` | Windows FILETIME ↔ `time_point` |
| `time::from_dos` / `to_dos` | DOS date/time (FAT/ZIP) ↔ `time_point` |
| `time::from_ntp` / `to_ntp` | NTP timestamp ↔ `time_point` |

---

### 6. Ranges (`range.hpp`)

Provides domain-safe, constexpr-friendly integer and strong-type ranges for iteration.

| Feature | Description |
|---------|------------|
| Forward / backward iteration | Controlled by `range_dir` |
| Inclusive / exclusive bounds | Controlled by `range_mode` |
| Step values | Customizable for loops |
| Strong type support | Iterates over `off_s`, `rva_s`, etc., preserving type safety |

---


---

## Integration

>[!IMPORTANT]
> C++ Modules require CMake 3.28+ / Xmake 2.8.1+ and a compatible compiler (Clang 16+, GCC 14+, or MSVC 19.34+).

### CMake

**Header-only (default) — clone manually:**

```cmake
add_subdirectory(extern/stx)
target_link_libraries(<target> PRIVATE lbyte::stx)
```

**C++ Modules — clone manually:**

```cmake
set(LBYTE_STX_USE_MODULES ON CACHE BOOL "" FORCE)
add_subdirectory(extern/stx)
target_link_libraries(<target> PRIVATE lbyte::stx)
```

**FetchContent (header-only):**

```cmake
include(FetchContent)
FetchContent_Declare(
    stx
    GIT_REPOSITORY https://github.com/zethcxx/stx.git
    GIT_TAG        v1.0.0
)
FetchContent_MakeAvailable(stx)
target_link_libraries(<target> PRIVATE lbyte::stx)
```

**FetchContent (modules):**

```cmake
include(FetchContent)
FetchContent_Declare(
    stx
    GIT_REPOSITORY https://github.com/zethcxx/stx.git
    GIT_TAG        v1.0.0
)
set(LBYTE_STX_USE_MODULES ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(stx)
target_link_libraries(<target> PRIVATE lbyte::stx)
```

### Xmake

**Package (auto-download from GitHub):**

```lua
package("zethcxx.stx")
    set_kind("library", { headeronly = true })
    set_urls("https://github.com/zethcxx/stx.git")
    add_versions("v1.0.0", "v1.0.0")

    add_configs("use_modules", {
        description = "Enable C++ Modules support",
        default = false,
        type = "boolean"
    })

    on_load(function (package)
        if package:config("use_modules") then
            package:add("cxxmodules", "modules/stx/*.cppm")
        end
        package:add("includedirs", "include")
    end)

    on_install(function (package)
        import("package.tools.xmake").install(package, {
            use_modules = package:config("use_modules")
        })
    end)

    on_test(function (package)
        package:check_cxxsnippets({ test = [[
            import lbyte.stx;
            int main() { return 0; }
        ]]}, { configs = { languages = "c++23" }})
    end)
package_end()

-- Header-only:
add_requires("zethcxx.stx")

-- Or with modules:
-- add_requires("zethcxx.stx", { configs = { use_modules = true }})

target("myapp")
    set_languages("cxx23")
    add_packages("zethcxx.stx")
```

**Clone manually — header-only:**

```lua
add_subdirs("stx")

target("myapp")
    set_languages("cxx23")
    add_deps("stx")
```

**Clone manually — modules:**

```lua
add_subdirs("stx", { configs = { use_modules = true }})

target("myapp")
    set_languages("cxx23")
    add_deps("stx")
    add_policy("build.c++.modules", true)
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

