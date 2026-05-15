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

| Member | Returns | Description |
|--------|---------|-------------|
| `raw()` | `T*` | Typed pointer to the object |
| `addr()` | `uptr` | Address as unsigned integer |
| `operator bool` | `bool` | Non-null check |
| `operator uptr` | `uptr` | Implicit conversion to `uptr` |

### Address Rebind

| Member | Description |
|--------|-------------|
| `operator=(address_like)` | Change pointed address |
| `as_p<U>()` | Rebind to `ptr<U>` |
| `as_w<U>()` | Rebind to `wptr<U>` |

### Typed Access

| Member | Description |
|--------|-------------|
| `operator->()` | Pointer-style member access |

### Binary Read / Write

| Member | Requirements | Description |
|--------|-------------|-------------|
| `read(off)` | — | Read pointer-sized value at offset, returns `ptr<T>` |
| `read<U>(off)` | `binary_readable<U>`, non-void | Copy-based read of type `U` with optional offset |
| `write(off, val)` | `binary_readable<U>`, non-void | Copy-based write with offset |
| `write(val)` | `binary_readable<U>`, non-void | Copy-based write at address |

### Alignment

| Member | Returns | Description |
|--------|---------|-------------|
| `align_up(alignment)` | `ptr` | Round address up to alignment boundary |
| `align_down(alignment)` | `ptr` | Round address down to alignment boundary |

### Alignment Check

| Member | Returns | Description |
|--------|---------|-------------|
| `is_aligned<T>()` | `bool` | Address aligned to `alignof(T)` |
| `is_aligned<N>()` | `bool` | Address aligned to `N` bytes |

### Arithmetic

| Member | Returns | Description |
|--------|---------|-------------|
| `add(off_s)` | `ptr` | Advance address by signed offset |
| `add(rva_s)` | `ptr` | Advance address by RVA |
| `sub(off_s)` | `ptr` | Rewind address by signed offset |
| `operator+(off_s)` | `ptr` | Advance address by signed offset |
| `operator+(rva_s)` | `ptr` | Advance address by RVA |
| `operator-(off_s)` | `ptr` | Rewind address by signed offset |
| `operator+=(off_s)` | `ptr&` | Advance in-place |
| `operator-=(off_s)` | `ptr&` | Rewind in-place |

### Distance

| Member | Returns | Description |
|--------|---------|-------------|
| `diff(ptr)` | `off_s` | Signed distance between two pointers |
| `operator-(ptr)` | `off_s` | Signed distance (alias) |

### Strong Type Conversions

| Member | Returns | Description |
|--------|---------|-------------|
| `off()` | `off_s` | Address as signed offset |
| `rva()` | `rva_s` | Address as 32-bit RVA |
| `va()` | `va_s` | Address as strong virtual address |

### Swap

| Member | Description |
|--------|-------------|
| `swap(ptr&)` | Exchange addresses (member) |
| `swap(a, b)` | Exchange (ADL friend) |

### Hash (std)

```cpp
std::unordered_set<ptr<T>> set;  // enabled via std::hash<ptr<T>>
```

### Call

| Member | Description |
|--------|-------------|
| `call<Sig>()` | Invoke address as function with signature `Sig` |

### Example

```cpp
stx::ptr<IMAGE_DOS_HEADER> dos{some_addr};

auto magic = dos->e_magic;                 // member access
auto ptr  = dos.raw();                     // IMAGE_DOS_HEADER*
auto addr = dos.addr();                    // uptr
dos = another_addr;                         // rebind
auto val = dos.read<stx::u32>(off_s{8});    // read with offset
dos.write<stx::u32>(off_s{12}, 0xDEAD);     // write with offset
dos.write<stx::u32>(0xBEEF);                // write at address

stx::ptr<void> vp{some_addr};
auto ip = vp.as_p<int>();  // ptr<int>
```

---

## `wptr<T, Stride>`

Walk pointer — non-owning pointer with offset arithmetic and pointer-chasing (walk/chain) operations.

```cpp
template<typename T = uptr, uptr Stride = 1>
class wptr
{
    uptr address = 0;
public:
    using value_type = T;

    constexpr wptr() noexcept = default;
    constexpr explicit wptr(address_like auto addr) noexcept;
};
```

`Stride` es un parámetro de plantilla (compile-time, sin runtime overhead).  
`wptr<T>::operator[](off)` ≡ `read<uptr>(address + off * Stride)`.

### Base Operations

| Member | Returns | Description |
|--------|---------|-------------|
| `raw()` | `T*` | Typed pointer to the object |
| `addr()` | `uptr` | Address as unsigned integer |
| `ref()` | `uptr&` | Mutable reference to address (para inline asm) |
| `operator bool` | `bool` | Non-null check |
| `operator uptr` | `uptr` | Implicit conversion to `uptr` |

### Dereference & Arrow Access

| Member | Returns | Description |
|--------|---------|-------------|
| `operator*()` | `T&` / `const T&` | Direct dereference |
| `operator[](off_s)` | `T&` / `const T&` | Raw dereference at byte offset |
| `operator->()` | `T*` | Typed member access |

### Binary Read / Write

| Member | Requirements | Description |
|--------|-------------|-------------|
| `read(off)` | — | Read pointer-sized value at offset, returns `ptr<T>` (inherited) |
| `read<U>(off)` | `binary_readable<U>`, non-void | Copy-based typed read with optional byte offset |
| `write(off, val)` | `binary_readable<U>`, non-void | Copy-based typed write with byte offset |
| `write(val)` | `binary_readable<U>`, non-void | Copy-based typed write at address |
| `call<Sig>()` | — | Invoke address as function with signature `Sig` |

### Example

```cpp
stx::wptr<void> base{0x140000000};

// Address
auto addr = base.addr();                  // uptr
auto ptr  = base.raw();                   // void*

// Offset
auto next = base + stx::off_s{0x100};

// Pointer chasing
stx::wptr<uptr> entry = base[0x10][0x20][0x08];  // preserva uptr

// raw() devuelve T*: wptr<uptr>::raw() → uptr*
stx::uptr* p = entry.raw();

// read<u32>() para leer valor en la direccion final
stx::u32 val = entry.read<stx::u32>();

// Typed member access (operator->)
struct S { int x; };
wptr<S> p = base.as_w<S>();
int mx = p->x;

// Read / Write
stx::u32 val = base.read<stx::u32>();
base.write(off_s{8}, stx::u16{0x1234});

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

auto raw_ptr = p.raw();           // int*
auto addr    = p.addr();          // uptr
p->x = 10;                        // member access
p = another_addr;                 // rebind
auto val = p.read<stx::u32>();    // binary read of u32
auto ptr = p.read(stx::off_s{8}); // read pointer at offset, returns ptr<int>
p.write(42);                      // write value
auto cp = p.as_p<short>();        // type rebind

auto aligned = p.align_up(16);    // align up to 16-byte boundary
auto off     = p.off();           // address as signed offset
auto rva     = p.rva();           // address as 32-bit RVA

auto next = p.add(stx::off_s{8}); // advance by 8 bytes
auto prev = p.sub(stx::off_s{4}); // rewind by 4 bytes
auto same = p + stx::off_s{8};    // operator+ overload
auto dist = next.diff(p);         // signed distance: off_s{8}

p += stx::off_s{16};              // advance in-place (ptr&)
p -= stx::off_s{4};               // rewind in-place

auto back = q - stx::off_s{32};   // operator- returns ptr
auto diff = q - p;                // ptr - ptr = off_s

auto& ref = *p;                   // dereference: T&
std::swap(a, b);                  // swap via ADL
auto h = std::hash<ptr<int>>{}(p); // hash for unordered containers

if (p.is_aligned<int>())    { }   // aligned to alignof(int)
if (p.is_aligned<16>())     { }   // aligned to 16 bytes
```

---

## Walk Pointer (Chaining)

```cpp
stx::wptr<uptr> root{0x140000000};

// cada [] ≡ read<uptr>(base + offset), preserva wptr<uptr,1>
stx::uptr target = root[0x10][0x20][0x08].addr();

// stride compile-time via template
stx::uptr v2 = root.at<4>()[3][5].addr();
// [3] → read(addr+3*4)
// [5] → read(addr'+5*4)
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
