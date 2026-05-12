# mem.hpp

## Overview

`mem.hpp` provides low-level memory access primitives and bit/align utilities for the `stx` namespace under C++23.

It defines:

- Copy-based typed memory reads and writes (well-defined behavior).
- Raw pointer-based reads and writes (direct dereference).
- Casting helpers (`rcast`, `scast`, `bcast`).
- Alignment utilities for integral and strong types.

The module is intended for controlled low-level environments such as:

- Binary parsing
- Manual mapping
- Memory inspection
- Instrumentation
- Runtime patching

All functions assume the caller guarantees memory validity, lifetime correctness, and access permissions.

---

## Force Inline Macro

| Macro | Meaning |
|--------|----------|
| `STX_FORCE_INLINE` | Expands to `[[gnu::always_inline]] inline` on GCC/Clang, otherwise `inline` |

Used to ensure minimal abstraction overhead in critical paths.

---

# Typed Memory Access

## 1. Copy-Based Read

```cpp
template<class Type, address_like Addr>
[[nodiscard]] STX_FORCE_INLINE
Type read(Addr base, off_s off = off_s{0}) noexcept;
```

### Semantics

- Uses `std::memcpy`.
- Behavior is defined under the C++ object model.
- Safe for unaligned memory.
- Safe under strict aliasing rules.
- Requires `std::is_trivially_copyable_v<Type>`.

### When to Use

| Scenario | Recommended |
|----------|-------------|
| Parsing binary structures | Yes |
| Packed / unaligned memory | Yes |
| Portable code | Yes |
| Memory-mapped files | Yes |

---

## 2. Raw Read (Direct Dereference)

```cpp
template<class Type>
[[nodiscard]] STX_FORCE_INLINE
Type read_raw(address_like auto base,
              off_s off = off_s{0}) noexcept;
```

### Semantics

- Direct pointer dereference.
- No `memcpy`.
- Requires aligned memory.
- May violate strict aliasing if misused.
- Requires `std::is_trivially_copyable_v<Type>`.

### When to Use

| Scenario | Recommended |
|----------|-------------|
| Known-aligned internal buffers | Yes |
| Performance-critical tight loops | Yes |
| Arbitrary mapped memory | No |

---

# Typed Memory Write

## 3. Copy-Based Write

```cpp
template<class Type, address_like Addr>
STX_FORCE_INLINE
void write(Addr base, off_s off, Type value) noexcept;
```

### Semantics

- Uses `std::memcpy`.
- Defined behavior.
- Safe for unaligned memory.
- Requires trivially copyable type.

---

## 4. Raw Write (Direct Dereference)

```cpp
template<class Type>
STX_FORCE_INLINE
void write_raw(address_like auto base,
               off_s off,
               Type value) noexcept;
```

### Semantics

- Direct assignment through cast pointer.
- Requires valid alignment.
- No aliasing guarantees.
- Suitable for controlled memory regions.

---

# Casting Utilities

## reinterpret_cast Wrapper

```cpp
template<class Type>
STX_FORCE_INLINE
Type rcast(auto value) noexcept;
```

Thin wrapper over `reinterpret_cast`.

Used to centralize casting style and improve readability in low-level code.

---

## static_cast Wrapper

```cpp
template<class Type>
STX_FORCE_INLINE
constexpr Type scast(auto value) noexcept;
```

Thin wrapper over `static_cast`.

Primarily used for:

- Narrowing control
- Integral conversions
- Enum conversions

---

## Bit Cast

```cpp
template<typename To, typename From>
[[nodiscard]] STX_FORCE_INLINE
constexpr To bcast(const From& from) noexcept;
```

Wrapper over `std::bit_cast`.

### Requirements

| Constraint | Description |
|------------|-------------|
| `sizeof(To) == sizeof(From)` | Required by `std::bit_cast` |
| Trivially copyable types | Required |

Used for safe reinterpretation of object representation.

---

# Alignment Utilities

## Strong Type Alignment

> **Note:** Integral alignment functions (`align_up`, `align_down`) are defined in `core.hpp`.

Provides alignment operations that preserve strong type semantics:

```cpp
template<typename T, typename Tag>
constexpr auto align_up(details::strong_type<T, Tag> st, T alignment) noexcept;

template<typename T, typename Tag>
constexpr auto align_down(details::strong_type<T, Tag> st, T alignment) noexcept;
```

Preserves strong type domain.

| Input | Output |
|--------|--------|
| `off_s` | `off_s` |
| `rva_s` | `rva_s` |
| `va_s` | `va_s` |

No domain leakage occurs.

---

# Typed Pointer Wrappers

## `ptr<T>`

Non-owning typed pointer. Stores a normalized address and provides typed access to the underlying object.

```cpp
template<typename T = void>
class ptr
{
    uptr address = 0;
public:
    using value_type = T;

    constexpr ptr() noexcept = default;
    constexpr explicit ptr(address_like auto addr) noexcept;
};
```

### Base Operations

| Member | Description |
|--------|-------------|
| `raw()` | Returns underlying `uptr` |
| `operator bool` | Non-null check |
| `operator uptr` | Implicit conversion to `uptr` |

### Address Rebind

| Member | Description |
|--------|-------------|
| `operator=(address_like)` | Change pointed address |
| `as<U>()` | Rebind to `ptr<U>` |
| `as_w<U>()` | Rebind to `wptr<U>` |

### Typed Access

| Member | Requirements | Description |
|--------|-------------|-------------|
| `operator->()` | `!std::is_void_v<T>` | Pointer-style member access |

### Binary Read / Write

| Member | Requirements | Description |
|--------|-------------|-------------|
| `read<T>(off)` | `binary_readable<T>` | Copy-based read with optional offset |
| `write<T>(off, val)` | `binary_readable<T>` | Copy-based write with offset |
| `write<T>(val)` | `binary_readable<T>` | Copy-based write at address |

### Call

| Member | Description |
|--------|-------------|
| `call<Sig>()` | Invoke address as function with signature `Sig` |

### Example

```cpp
stx::ptr<IMAGE_DOS_HEADER> dos{some_addr};

auto magic = dos->e_magic;               // member access
dos = another_addr;                       // rebind
auto val = dos.read<stx::u32>(off_s{8});  // read with offset
dos.write<stx::u32>(off_s{12}, 0xDEAD);   // write with offset
dos.write<stx::u32>(0xBEEF);              // write at address

// Void pointer, typed rebind
stx::ptr<void> vp{some_addr};
auto ip = vp.as<int>();  // ptr<int>
```

---

## `wptr<T>`

Walk pointer — non-owning pointer with offset arithmetic and pointer-chasing (walk/chain) operations.

```cpp
template<typename T = void>
class wptr
{
    uptr address = 0;
public:
    using value_type = T;

    constexpr wptr() noexcept = default;
    constexpr explicit wptr(address_like auto addr) noexcept;
};
```

### Base Operations

| Member | Description |
|--------|-------------|
| `raw()` | Returns underlying `uptr` |
| `operator bool` | Non-null check |
| `operator uptr` | Implicit conversion to `uptr` |

### Offset Arithmetic

| Member | Description |
|--------|-------------|
| `operator+(usize)` | Returns new `wptr` advanced by offset |
| `operator+(off_s)` | Returns new `wptr` advanced by strong offset |

### Walk / Chain (Pointer Chasing)

Reads a `uptr` from the target address and returns it as a new `wptr<>`.

| Member | Description |
|--------|-------------|
| `walk(offset)` | Dereference at `address + offset` as `uptr`, return `wptr<>` |
| `operator[](offset)` | Alias for `walk()` — supports chain syntax |

```cpp
stx::wptr<> base{0x140000000};

// chain: read pointer at base+0x100, then at that address + 0x20
stx::wptr<> target = base[0x100][0x20];
```

### Rebind

| Member | Description |
|--------|-------------|
| `as<U>()` | Rebind to `ptr<U>` |
| `as_w<U>()` | Rebind to `wptr<U>` |

### Direct Read

| Member | Requirements | Description |
|--------|-------------|-------------|
| `read<U>()` | `binary_readable<U>`, `U` non-void | Copy-based typed read from address |
| `call<Sig>()` | — | Invoke address as function with signature `Sig` |

### Example

```cpp
stx::wptr<void> base{0x140000000};

// Offset
auto next = base + stx::off_s{0x100};

// Pointer chasing
stx::wptr<> entry = base[0x10][0x20][0x08];

// Typed rebind
stx::ptr<int> p = base.as<int>();

// Read value
stx::u32 val = base.read<stx::u32>();

// Call as function
auto result = base.call<int(int, int)>(10, 20);
```

---

# Safety Model

This header assumes:

- Caller ensures region validity.
- No bounds checking is performed.
- No lifetime validation.
- No race condition protection.

| Responsibility | Owner |
|---------------|-------|
| Memory validity | Caller |
| Alignment correctness | Caller |
| Concurrency safety | Caller |

---

# Design Characteristics

- Header-only.
- Zero dynamic allocation.
- constexpr-friendly where possible.
- Explicit separation between defined and raw memory semantics.
- Strong type compatibility (`off_s`, `rva_s`, `va_s`).
- Integral alignment utilities provided by `core.hpp`.
- C++23 compliant.

---

# Example Usage

---

## Safe Read

```cpp
struct header
{
    stx::u32 magic;
    stx::u16 version;
    stx::u16 flags;
};

stx::va_s base{0x140000000};

header h = stx::read<header>(base, stx::off_s{0x100});
```

---

## Raw Read

```cpp
stx::u32 value =
    stx::read_raw<stx::u32>(base, stx::off_s{0x200});
```

---

## Write

```cpp
stx::write<stx::u32>(base, stx::off_s{0x300}, 0xDEADBEEF);
```

---

## Strong Type Alignment

```cpp
stx::off_s off{123};
auto aligned = stx::align_up(off, 16);  // returns off_s{128}
```

---

## Typed Pointer

```cpp
stx::ptr<int> p{some_address};

p->x = 10;            // member access
p = another_addr;     // rebind
auto val = p.read<stx::u32>();  // binary read
p.write(42);          // write value
auto cp = p.as<short>();  // type rebind
```

---

## Walk Pointer (Chaining)

```cpp
stx::wptr<void> root{0x140000000};

// Pointer chase: read uptr at root+0x10,
// then at result+0x20, then at result+0x08
stx::wptr<> target = root[0x10][0x20][0x08];

// Typed read at final address
stx::u32 value = target.read<stx::u32>();
```

---

## Call Through Pointer

```cpp
stx::ptr<void> p{function_address};

int result = p.call<int(int, int)>(10, 20);
```

---

# Intended Usage Domain

- PE/ELF/Mach-O loaders
- Memory inspection tools
- Runtime patching frameworks
- Red team internal tooling
- Systems-level diagnostics

`mem.hpp` provides a controlled abstraction layer for raw memory manipulation while preserving C++23 compile-time guarantees where applicable.
