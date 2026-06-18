# STX — C++23 Systems Toolbelt
> Disclaimer: This project is intended for personal use and experimentation. Users are free to fork or modify it, but all usage is at their own risk. The author provides no guarantees regarding functionality, security, or safety.

**Version:** 0.1.0

A header-only C++23 library for low-level systems programming, binary analysis, and runtime instrumentation. Includes compile-time string literals, time utilities, and integer ranges.

---

## Technical Specifications

| Feature              | Description                                               |
|----------------------|-----------------------------------------------------------|
| Language Standard    | C++23                                                     |
| Header-only          | Yes                                                       |
| C++ Modules          | Yes                                                       |
| Dependencies         | Standard library only                                     |
| Target Domains       | Binary analysis, runtime patching, low-level tooling      |

---

## Modules (`lbyte::stx`)

Include `<lbyte/stx.hpp>` for all modules or individual headers.

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

### 3. File System (`io.hpp`)

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

### 7. String Literals (`ct.hpp`)

#### Namespaces

| Namespace     | Contents                                      |
|---------------|-----------------------------------------------|
| `ct::fmt`     | String formatting flags (`strip`, `unindent`) |
| `ct::endian`  | Endianness (`order::little`, `order::big`)    |

#### Components

| Component                        | Description                                         |
|----------------------------------|-----------------------------------------------------|
| `ct::str<"...", fmt...>`     | Compile-time string transform, `.rodata` storage    |
| `ct::fixed_string<N>`                    | Fixed-string NTTP for your own templates            |
| `ct::fmt::strip / unindent`      | Transform flags                                     |
| `ct::istr<"...", endian?>`       | Native integer (u8/u16/u32/u64), N ≤ 8              |
| `ct::vstr<"...">`                | `byte_block<N>` with `.data()` / `.size()`, any N   |
| `ct::byte_block<N>`              | Raw byte array with `.data()` / `.size()`           |

### 8. Time (`time.hpp`)

| Component                              | Description                                    |
|----------------------------------------|------------------------------------------------|
| `time::from_unix<Dur>` / `to_unix`     | UNIX timestamp ↔ `time_point`                  |
| `time::now()` / `now_ms()` / `now_ns()`| Current UNIX time                              |
| `time::stopwatch`                      | Monotonic timer with `lap()` and `reset()`     |
| `time::from_filetime` / `to_filetime`  | Windows FILETIME ↔ `time_point`                |
| `time::from_dos` / `to_dos`            | DOS date/time (FAT/ZIP) ↔ `time_point`         |
| `time::from_ntp` / `to_ntp`            | NTP timestamp ↔ `time_point`                   |

### 9. Range (`range.hpp`)

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

**Header-only:**

```cmake
add_subdirectory(extern/stx)
target_link_libraries(<target> PRIVATE lbyte::stx)
```

**With Modules:**

```cmake
set(LBYTE_STX_USE_MODULES ON CACHE BOOL "" FORCE)
add_subdirectory(extern/stx)
target_link_libraries(<target> PRIVATE lbyte::stx)
```

**FetchContent:**

```cmake
include(FetchContent)
FetchContent_Declare(
    stx
    GIT_REPOSITORY https://github.com/zethcxx/stx.git
    GIT_TAG        v0.1.0
)
FetchContent_MakeAvailable(stx)
target_link_libraries(<target> PRIVATE lbyte::stx)
```

### Xmake

**Fetch from git:** Create a package script at `packages/z/zethcxx.stx/xmake.lua`:

```lua
package("zethcxx.stx")
    set_kind("library", {headeronly = true})
    set_homepage("https://github.com/zethcxx/stx")
    set_description("C++23 Systems Toolbelt")

    add_urls("https://github.com/zethcxx/stx.git")
    add_versions("v0.1.0", "v0.1.0")

    add_configs("use_modules",  { description = "Build C++ modules", default = false, type = "boolean" })

    on_load(function (package)
        package:add("includedirs", "include")
        if package:config("use_modules") then
            package:add("cxxmodules", "modules/stx/*.cppm")
        end
    end)

    on_install( function( package )
        local configs = {}
        local includedir = package:installdir("include")

        if package:config( "use_modules" ) then
            configs.use_modules = true
        end

        import("package.tools.xmake").install( package, configs, { includedirs = includedir })
    end)

    on_test(function (package)
        assert(package:check_cxxsnippets({test = [[
            #include <lbyte/stx/core.hpp>
            using namespace lbyte::stx;
            static_assert(version.major == 0);
        ]]}, { configs = { languages = "cxx23" } }))
    end)
package_end()
```

Then in your project's `xmake.lua`:

```lua
add_requires("zethcxx.stx")

target("myapp")
    set_languages("cxx23")
    add_packages("zethcxx.stx")
```

**Local copy (git clone / submodule):**

```lua
add_subdirs("stx")

target("myapp")
    set_languages("cxx23")
    add_deps("stx")
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
