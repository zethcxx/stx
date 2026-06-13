# STX / ZOU — C++23 Systems Toolbelt
> Disclaimer: This project is intended for personal use and experimentation. Users are free to fork or modify it, but all usage is at their own risk. The author provides no guarantees regarding functionality, security, or safety.

**Version:** 0.1.0

Two header-only C++23 libraries for low-level systems programming, binary analysis, and runtime instrumentation.

- **STX** (`lbyte::stx`) — core: types, memory, functions, filesystem, bit/endian, literals.
- **ZOU** (`lbyte::zou`) — optional compile-time utilities: strings, time, ranges. Zero-overhead utils.

---

## Technical Specifications

| Feature              | Description                                               |
|----------------------|-----------------------------------------------------------|
| Language Standard    | C++23                                                     |
| Header-only          | Yes (STX mandatory, ZOU optional)                         |
| C++ Modules          | Yes (both targets)                                        |
| Dependencies         | Standard library only                                     |
| Target Domains       | Binary analysis, runtime patching, low-level tooling      |

---

## STX Modules (`lbyte::stx`)

All always available. Include `<lbyte/stx.hpp>` or individual headers.

### 1. Core Types (`core.hpp`)

| Type / Concept      | Description                                              |
|---------------------|----------------------------------------------------------|
| `off_s`             | Strong offset type (replaces legacy `off_t`)             |
| `rva_s`             | Strong RVA                                               |
| `va_s`              | Strong virtual address                                   |
| `address_like`      | Concept for address types                                |
| `binary_readable`   | Concept for binary-safe types                            |

### 2. Memory Utilities (`mem.hpp`)

| Component                      | Description                                              |
|--------------------------------|----------------------------------------------------------|
| `read<Type>(addr, offset)`     | Copy-based read; safe for unaligned memory               |
| `read_raw<Type>(addr, offset)` | Direct dereference; requires alignment                   |
| `write<Type>(addr, offset, v)` | Copy-based write                                         |
| `ptr<T>`                       | Typed non-owning pointer with `->`, `raw()`, `read/write`|
| `mem::align_up / align_down`   | Power-of-two alignment                                   |
| `mem::gap_v / mem::gap_align_v`| Compile-time size calculators                            |

### 3. File System (`fs.hpp`)

| Function                             | Description                                      |
|--------------------------------------|--------------------------------------------------|
| `readfs<Type>(file, offset, dir)`    | Read single object from `std::istream`           |
| `readfs<Type>(file, offset, n, dir)` | Read multiple objects into `dirty_vector`        |
| `setposfs(file, offset, dir)`        | Strongly-typed stream positioning                |
| `skipfs(file, offset)`               | Forward stream by `offset` elements              |

### 4. Function Abstractions (`fn.hpp`)

| Component               | Description                                        |
|-------------------------|----------------------------------------------------|
| `caller_t<Sig>`         | Wraps function pointer with compile-time signature |
| `caller<Sig>(addr)`     | Factory to produce a `caller_t`                    |

### 5. Bit & Endian (`bit.hpp`, `endian.hpp`)

Bit manipulation and endian conversion utilities.

### 6. Literals (`literals.hpp`)

User-defined literals for strong types and units.

---

## ZOU Modules (`lbyte::zou` — optional)

Zero-overhead compile-time utilities. Gated by `LBYTE_STX_BUILD_ZOU` (CMake) / `with_zou` (Xmake), both default ON.

Include `<lbyte/zou.hpp>` or individual headers.

### 1. String Literals (`str.hpp`)

| Component                       | Description                                        |
|---------------------------------|----------------------------------------------------|
| `lit::str<"...", flags...>`     | Compile-time string transform, `.rodata` storage   |
| `lit::fstr<N>`                  | Fixed-string NTTP for your own templates           |
| `lit::strip` / `lit::unindent`  | Transform flags                                    |

### 2. Time (`time.hpp`)

| Component                              | Description                                    |
|----------------------------------------|------------------------------------------------|
| `time::from_unix<Dur>` / `to_unix`     | UNIX timestamp ↔ `time_point`                  |
| `time::now()` / `now_ms()` / `now_ns()`| Current UNIX time                              |
| `time::stopwatch`                      | Monotonic timer with `lap()` and `reset()`     |
| `time::from_filetime` / `to_filetime`  | Windows FILETIME ↔ `time_point`                |
| `time::from_dos` / `to_dos`            | DOS date/time (FAT/ZIP) ↔ `time_point`         |
| `time::from_ntp` / `to_ntp`            | NTP timestamp ↔ `time_point`                   |

### 3. Range (`range.hpp`)

| Component        | Description                                              |
|------------------|----------------------------------------------------------|
| `range<T>(...)`  | Exclusive integer / strong-type range                    |
| `irange<T>(...)` | Inclusive integer / strong-type range                    |
| `range_mode`     | Boundary policy (`Inclusive` / `Exclusive`)              |

Supports forward/backward, custom step, enums, strong types.

---

## Integration

> [!IMPORTANT]
> C++ Modules require CMake 3.28+ / Xmake 2.8.1+ and a compatible compiler (Clang 16+, GCC 14+, or MSVC 19.34+).

### CMake

**Header-only (STX + ZOU):**

```cmake
add_subdirectory(extern/stx)
target_link_libraries(<target> PRIVATE lbyte::stx lbyte::zou)
```

**STX only (no ZOU):**

```cmake
set(LBYTE_STX_BUILD_ZOU OFF CACHE BOOL "" FORCE)
add_subdirectory(extern/stx)
target_link_libraries(<target> PRIVATE lbyte::stx)
```

**With Modules:**

```cmake
set(LBYTE_STX_USE_MODULES ON CACHE BOOL "" FORCE)
add_subdirectory(extern/stx)
target_link_libraries(<target> PRIVATE lbyte::stx lbyte::zou)
```

**FetchContent:**

```cmake
include(FetchContent)
FetchContent_Declare(
    stx
    GIT_REPOSITORY https://github.com/zethcxx/stx.git
    GIT_TAG        v0.1.0
)
set(LBYTE_STX_BUILD_ZOU ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(stx)
target_link_libraries(<target> PRIVATE lbyte::stx lbyte::zou)
```

### Xmake

**Clone manually — header-only:**

```lua
add_subdirs("stx")

target("myapp")
    set_languages("cxx23")
    add_deps("stx", "zou")
```

**STX only:**

```lua
add_subdirs("stx", { configs = { with_zou = false }})

target("myapp")
    set_languages("cxx23")
    add_deps("stx")
```

**With Modules:**

```lua
add_subdirs("stx", { configs = { use_modules = true, with_zou = true }})

target("myapp")
    set_languages("cxx23")
    add_deps("stx", "zou")
    add_policy("build.c++.modules", true)
```

---

## Troubleshooting

### Missing Clang Resource Headers

When using C++ Modules with Clang, you might encounter errors like:

```
fatal error: 'stddef.h' file not found
```

Export the resource path:

```bash
export CPLUS_INCLUDE_PATH="/usr/lib/clang/21/include/":$CPLUS_INCLUDE_PATH
```

---

## Design Principles

- Header-only, zero-runtime overhead abstractions
- Strong typing for offsets, addresses, and function signatures
- Explicit memory and file safety, no hidden side effects
- C++23 constexpr-friendly, usable in compile-time contexts
- Focused on low-level tooling, scripting, reverse engineering
