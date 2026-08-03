# STX — C++23 Systems Toolbelt
> Disclaimer: This project is intended for personal use and experimentation. Users are free to fork or modify it, but all usage is at their own risk. The author provides no guarantees regarding functionality, security, or safety.

**Version:** 0.2.1

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
| `ct::endian`  | Endianness (enum `v`, type `tag<E>`, aliases `little`/`big`) |

#### Components

| Component                        | Description                                         |
|----------------------------------|-----------------------------------------------------|
| `ct::str<"...", fmt...>`     | Compile-time string transform, `.rodata` storage    |
| `ct::fixed_string<N>`                    | Fixed-string NTTP for your own templates            |
| `ct::fmt::strip / unindent`      | Transform flags (includes `trim_left`, `replace_all`, `chain`, etc.) |
| `ct::fmt::remove_blank_lines` | Remove blank/whitespace-only lines |
| `ct::args<Vs...>`                | Format string expansion (`{}`, `{:x}`, `{:>8}`)     |
| `ct::str_type<N>`                | Underlying type of `ct::str` with `apply<MoreFlags...>()` |
| `ct::istr<"...", T?, Order?>`    | Integral string (auto/explicit type, little/big endian), N ≤ 8 |
| `ct::vstr<"...">` / `vstr<"...", N>` | `byte_block<N>` with `.data()` / `.size()`, padded to N |
| `ct::byte_block<N>`              | `std::array<u8, N>` (`.data()`, `.size()`, iteration) |
| `ct::repeat<V, Reps>`               | Repeat pattern V (scalar/array), `Reps` times → `std::array` |

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

### 10. Cycle (`cycle.hpp`)

| Component          | Description                                              |
|--------------------|----------------------------------------------------------|
| `cycle(r)`         | Infinite repetition — `break` or compose to stop         |
| `cycle(r, n)`      | Exactly `n` passes over the underlying range             |

Sentinel-based, `constexpr`, works with `range` and standard containers; models `std::ranges::view` / `input_range`, so it composes with `std::views` adaptors like `views::zip`. Nothing in C++23/26; proposed for C++29 (`views::cycle`).

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
    GIT_TAG        v0.2.1
)
FetchContent_MakeAvailable(stx)
target_link_libraries(<target> PRIVATE lbyte::stx)
```

### Xmake

**Fetch from git:** Create a package script at `packages/l/lbyte.stx/xmake.lua`:

```lua
package("lbyte.stx")
    set_kind("library", {headeronly = true})
    set_homepage("https://github.com/zethcxx/stx")
    set_description("C++23 Systems Toolbelt")

    add_urls("https://github.com/zethcxx/stx.git")
    add_versions("main", "main")
    add_versions("v0.1.0", "v0.1.0")
    add_versions("v0.2.0", "v0.2.0")
    add_versions("v0.2.1", "v0.2.1")

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
            using namespace lbyte;

            int main(){ return stx::u32{}; }
        ]]}, { configs = { languages = "cxx23" } }))
    end)
package_end()
```

Then in your project's `xmake.lua`:

```lua
add_requires("lbyte.stx")

target("myapp")
    set_languages("cxx23")
    add_packages("lbyte.stx")
```

**Local copy (git clone / submodule):**

```lua
add_subdirs("stx")

target("myapp")
    set_languages("cxx23")
    add_deps("stx")
```

---

## Design Principles

### Pay for what you use

No global state, no vtable, no hidden allocations, no registration. If you don't include a header, it doesn't exist. If you include it, the cost is predictable:

| Area                 | Cost model                          | Notes                                                         |
|----------------------|-------------------------------------|---------------------------------------------------------------|
| `ct::`               | **Zero** — compile-time only        | `constexpr` / `consteval` — disappears entirely at runtime    |
| `mem::ptr`, `memcur` | **O(1)**, no hidden work            | Plain pointer arithmetic + `memcpy`; `constexpr`-safe helpers |
| `mem::read / write`  | **O(1)**                            | Single `memcpy` or aligned dereference                        |
| `io::readfs`         | **O(n)**, no heap                   | User-provided buffer, no hidden alloc                         |
| `time::now()`        | **1 syscall**                       | Wraps `clock_gettime`                                         |
| `range`, `fn`        | **Zero** — all `constexpr` / inline | Optimizer folds them away                     |
| `cycle`              | **Zero** — all `constexpr` / inline | Iterators + pass counter, inlined             |

### Strong typing

Offsets, addresses, and function signatures use distinct types — not raw integers. Impossible to accidentally pass an RVA where an offset is expected.

### Explicit over implicit

- `mem::read` always copies — no aliasing footguns
- `mem::read_raw` explicitly opts into direct dereference (requires alignment)
- Stream positioning requires an explicit direction (`beg`, `cur`, `end`)

### C++23 constexpr-friendly

All `ct::` and `range` components are `constexpr`. Use them in static_assert, template args, or as NTTPs.

### Focus

Low-level tooling, binary analysis, runtime patching, reverse engineering.

