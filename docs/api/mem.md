# mem.hpp

## Overview

`mem.hpp` provides low-level memory access primitives and bit/align utilities under `namespace mem` within `namespace lbyte::stx` for C++23.

It defines:

- Copy-based typed memory reads and writes (well-defined behavior), under `mem::read`, `mem::write`, etc.
- Raw pointer-based reads and writes (direct dereference), under `mem::read_raw`, `mem::write_raw`.
- Alignment utilities for integral and strong types.
- The `ptr<T, Stride>` typed address wrapper class (stays in `namespace stx`).

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

## `ptr<T, Stride>`

Stores a normalized address and provides typed access. `Stride` scales the pointer chase operator (`/`) for structured pointer chains (e.g. PE/ELF tables where entries are `N` bytes apart).

```cpp
template<typename T = void, uptr Stride = 1>
class ptr
{
    uptr address = 0;
public:
    using value_type = T;

    constexpr ptr() noexcept = default;
    constexpr explicit ptr(address_like auto addr) noexcept;
    ptr(T* raw_ptr) noexcept;   // non-constexpr, from raw pointer
};
```

### Base Operations

| Member | Returns | Description |
|--------|---------|-------------|
| `raw()` | `T*` | Typed pointer to the object |
| `addr()` | `uptr` | Address as unsigned integer |
| `operator bool` | `bool` | Non-null check |
| `operator uptr` | `uptr` | Explicit conversion to `uptr` |

### Null State

The library provides `stx::null` (type `null_t`) for null-pointer construction and comparison.

| Expression | Description |
|------------|-------------|
| `ptr{null}` | Construct null `ptr` (address = 0) |
| `p == null` | True if `p.addr() == 0` |
| `p != null` | True if `p.addr() != 0` |

Unlike `nullptr`, `stx::null` does not satisfy the `address_like` concept, so it cannot accidentally be passed to address-expecting APIs. The explicit `ptr(null_t)` constructor and comparison operators keep the interface safe and intentional.

```cpp
stx::ptr<int> p{stx::null};       // null ptr
if (p == stx::null) { /* empty */ }
```

### Address Rebind

| Member | Description |
|--------|-------------|
| `operator=(address_like)` | Rebind from address-like value |
| `operator=(T*)` | Rebind from raw pointer (non-constexpr) |
| `operator=(const ptr<T, OtherStride>&)` | Cross-stride copy — address only, stride type unchanged |
| `as_p<U>()` | Rebind to `ptr<U, 1>` |
| `as<U>()` | Cast address to `U` (e.g. `as<off_s>()`, `as<va_s>()`) |

### Dereference / Member Access

| Member | Requirements | Description |
|--------|-------------|-------------|
| `operator*()` | non-void `T` | Dereference: returns `T&` |
| `operator->()` | — | Member access via `T*` |

### Navigation

| Member | Returns | Description |
|--------|---------|-------------|
| `p[n]` (integral) | `ptr_light<T, Stride>` | Offset address by `n` bytes, return non-chainable proxy (no memory access) |
| `p[off]` (`byte_offset`) | `ptr_light<T, Stride>` | Same, via `off_s`/`rva_s` offset |

> `operator[]` is a **syntactic convenience** for `ptr + off` — it avoids extra parentheses: `ptr[off].method()` is visually cleaner than `(ptr + off).method()`. It does **not** dereference memory (unlike C arrays — no `*` is performed).

The return type `ptr_light<T>` extends `ptr<T>` but has `operator[]` deleted, making `ptr[a][b]` a **compile error**. This prevents misinterpretation as a multi-dimensional array when the operation is purely address arithmetic:

```cpp
ptr<void> base{address};
base[0x100]        // OK:  ptr_light at address + 0x100
base[0x100][0x20]  // ERROR: ptr_light::operator[] is deleted
(base + 0x100)[0x20]  // OK: (base + 0x100) returns ptr, then operator[] returns ptr_light
base[0x100].read<u32>() // OK: ptr_light inherits read/write/push/etc.
```

### Pointer Chase (`>>`)

| Expression | Returns | Description |
|-----------|---------|-------------|
| `p >> i` (integral) | `ptr<T, Stride>` | Enter `p` at offset `i × Stride`, read the value as `T` if non-void (else `uptr`), return new `ptr` wrapping the result |
| `p >> off` (`byte_offset`) | `ptr<T, Stride>` | Same, via `off.get()` |

Analogous to directory traversal: `ptr >> 0x10 >> 0x20` reads as *"enter ptr, walk 0x10 Stride entries, read the pointer-sized value there, enter the result, walk 0x20 Stride entries, read again"*.

Semantics (`void` case):
1. Target = `addr() + i*Stride`
2. `memcpy` a `uptr` from Target
3. Return `ptr<void, Stride>` wrapping the read value

For non-void `T`, reads `T` instead of `uptr` and returns `ptr<T, Stride>`.

Does **not** modify `*this`. All temporaries have the same `T` and `Stride` as the original.

```cpp
ptr<void> pe_base{0x140000000};

// Navigate, then chase
uptr target = (pe_base[0x100] >> 0x10 >> 0x20 >> 0x08).addr();

// stride via step<N>
uptr v2 = (pe_base.step<4>() >> 3 >> 5).addr();
// / 3 → reads uptr at addr + 3*4
// / 5 → reads uptr at result + 5*4
```

### Walk (Byte-Level Chase)

| Expression | Returns | Description |
|-----------|---------|-------------|
| `p.walk<Return>(off)` (`byte_offset`) | `ptr<Return, Stride>` | Read `Return` at `addr + off` (no stride scaling), return new `ptr` |
| `p.walk<Return>(n)` (integral) | `ptr<Return, Stride>` | Same, via implicit `off_s{n}` |
| `p.walk(off)` (`byte_offset`, default) | `ptr<void, Stride>` | Read `uptr`, return `ptr<void, Stride>` |

Templated return type: defaults to `ptr<void, Stride>` (reading `uptr`). Specify `Return` explicitly for typed reads.

Like `/` but ignores `Stride` — offset is an absolute byte offset, not multiplied.

### Stride Management

| Member | Returns | Description |
|--------|---------|-------------|
| `step<NewStride>()` | `ptr<T, NewStride>` | New `ptr` with stride `N` at same address (does not modify `*this`) |
| `step<U>()` | `ptr<T, sizeof(U)>` | New `ptr` with stride = `sizeof(U)` |

```cpp
ptr<int, 2> p{addr};
auto p2 = p.step<4>();              // ptr<int, 4> at same address
auto p3 = p.step<stx::u32>();       // ptr<int, 4> via sizeof(u32)
p = ptr<int, 4>{ p.addr() };      // cross-stride rebind in-place
```

### Read (no advance)

| Member | Returns | Requirements | Description |
|--------|---------|-------------|-------------|
| `read<T>()` | `T` | `binary_readable<T>` | Copy-based read at current address |
| `read<U>()` (bounded array) | `bounded_array_t<U>` | `bounded_array<U>` | Copy-based read of a bounded C array, returns `std::array` |
| `read_p<T>()` | `ptr<T, 1>` | non-void | Read a pointer value, returns `ptr<T>` |
| `read_le<T>()` | `T` | `byte_swappable<T>` | Little-endian read |
| `read_be<T>()` | `T` | `byte_swappable<T>` | Big-endian read |
| `as_view<U>()` (bounded array) | `span<const element_type>` | `bounded_array<U>` | Zero-copy view of a bounded C array at current address |
| `as_view<T>(count)` | `span<const T>` | `binary_readable<T>` | Zero-copy dynamic-extent view |
| `read_into(buf)` | `void` | `writable_buffer<R>` | Copy array/size bytes into writable buffer (no advance) |

### Pop (read + advance cursor)

| Member | Returns | Description |
|--------|---------|-------------|
| `pop<T>()` | `T` | Copy-based read, `address += sizeof(T)` |
| `pop<U>()` (bounded array) | `bounded_array_t<U>` | Copy-based read of a bounded C array, `address += sizeof(array)` |
| `pop_into(buf)` | `ptr&` | Copy bytes into writable buffer, `address += bytes` |

### Write (no advance)

| Member | Requirements | Description |
|--------|-------------|-------------|
| `write<T>(val)` | `binary_readable<T>`, not `contiguous_buffer` | Copy-based write of a scalar value at current address |
| `write(buf)` | `contiguous_buffer<R>` | Copy bytes from any contiguous range (span, vector, string_view) at current address |
| `write_le<T>(val)` | `byte_swappable<T>` | Little-endian write |
| `write_be<T>(val)` | `byte_swappable<T>` | Big-endian write |

### Push (write + advance cursor)

| Member | Returns | Description |
|--------|---------|-------------|
| `push<T>(val)` | `ptr&` | Write single scalar value, `address += sizeof(T)` |
| `push(buf)` | `ptr&` | Write bytes from any `contiguous_buffer`, `address += bytes` (unifies span, vector, string_view) |

### Unsafe (Direct Deref, no advance)

| Member | Requirements | Description |
|--------|-------------|-------------|
| `read_raw<T>()` | `binary_readable<T>` | Direct deref at current address |
| `write_raw(val)` | `binary_readable<T>` | Direct deref write at current address |

### Alignment

| Member | Returns | Description |
|--------|---------|-------------|
| `align_up(n)` | `ptr<T, Stride>` | Align address up to `n`-byte boundary |
| `align_down(n)` | `ptr<T, Stride>` | Align address down to `n`-byte boundary |
| `is_aligned<T>()` | `bool` | Check alignment to `alignof(T)` |
| `is_aligned<N>()` | `bool` | Check alignment to `N` |

### Arithmetic

| Expression | Returns | Description |
|-----------|---------|-------------|
| `p + off` | `ptr<T, Stride>` | New ptr at `addr + off` |
| `p - off` | `ptr<T, Stride>` | New ptr at `addr - off` |
| `p += off` | `ptr<T, Stride>&` | Advance in-place |
| `p -= off` | `ptr<T, Stride>&` | Rewind in-place |
| `p - q` | `off_s` | Signed distance |
| `p.diff(addr)` | `off_s` | Signed distance to any `address_like` (ptr, raw ptr, `uptr`, `va_s`) |

### Increment / Decrement

| Expression | Advance by |
|-----------|-----------|
| `++p` / `p++` | `sizeof(T) * Stride` (or `Stride` if `T` is `void`) |
| `--p` / `p--` | `sizeof(T) * Stride` (or `Stride` if `T` is `void`) |

Scales by both `sizeof(T)` and `Stride`. For `ptr<void, 1>`, advances by 1 byte (equivalent to old behavior).

### Call

| Member | Description |
|--------|-------------|
| `call<Sig>(args...)` | Invoke address as function with signature `Sig` |
| `caller<Sig>()` | Return reusable `caller_t<Sig>` without invoking |

### Other

| Member | Description |
|--------|-------------|
| `swap(p)` | Exchange addresses |
| `std::hash<ptr<T, Stride>>` | Hash by address, usable in `unordered_set`/`map` |
| `ptr_light<T, Stride>` | Non-chainable proxy returned by `operator[]`; inherits all `ptr` methods except `[]` |

### Example

```cpp
ptr<void> base{0x140000000};

// Address
auto addr = base.addr();                  // uptr
auto ptr  = base.raw();                   // void*

// Offset navigation
auto next = base[0x100];                  // ptr_light at offset 0x100 (no read, no chaining)
auto next2 = base + off_s{0x100};         // same, via operator+ returns ptr

// Pointer chase (directory traversal metaphor)
uptr target = (base[0x100] >> 0x10 >> 0x20).addr();

// Walk (byte offset, no stride)
uptr via_walk = (base.walk(off_s{8})).addr();

// Safe read/write — offset through operator[]
u32 val = base[off_s{0x200}].read<u32>();
base[off_s{8}].write(u16{0x1234});

// Chaining is prevented — use operator+ or compose in one statement
// base[off1][off2]              // ERROR: ptr_light::operator[] is deleted
// (base + off1)[off2]           // OK:   operator+ returns ptr, then []

// Stride management
auto with_stride = base.step<4>();          // ptr<void, 4>
uptr chased = (with_stride >> 3 >> 5).addr();

// Call as function
auto result = base.call<int(int, int)>(10, 20);

// Cross-stride assignment
ptr<int, 4> p{addr};
ptr<int, 8> q{addr};
p = q;  // address copied, p's stride stays 4
```

---

## C/C++ Comparison: `ptr` vs Raw Pointers

### `p[off]` is Not C Array Subscript

Unlike C, `ptr::operator[]` is pure address arithmetic — it never dereferences memory:

```cpp
// stx:  addr + off, no memory access
auto p = base[0x100];               // ptr_light at base + 0x100 (no read)
u32 val = base[0x100].read<u32>();  // then explicitly read
```

```c
// C:  [] always dereferences
void* p = (char*)addr + 0x100;      // C needs explicit cast + add
uint32_t val = *(uint32_t*)((char*)addr + 0x100);  // offset + deref in one step
```

```cpp
// C++ raw:  offset explicit, deref separate
uint32_t val;
std::memcpy(&val, (const char*)addr + 0x100, sizeof(val));
```

The return type `ptr_light` enforces this: `base[a][b]` is a compile error because `ptr_light::operator[]` is deleted. In C, `arr[a][b]` implies a 2D array with two dereferences — `ptr` avoids this semantic confusion entirely.

### Pointer Chase (`>>`)

```cpp
// stx: chainable, zero-cost
stx::ptr<void> base{0x140000000};
uptr target = (base[0x100] >> 0x10 >> 0x20).addr();
```

```c
// C equivalent: manual deread + offset
u64 tmp1 = *(u64*)((char*)0x140000000 + 0x100);
u64 tmp2 = *(u64*)((char*)tmp1 + 0x10);
u64 target = *(u64*)((char*)tmp2 + 0x20);
```

```cpp
// C++ raw equivalent: verbose, error-prone
u64 t1 = *reinterpret_cast<u64*>(0x140000000 + 0x100);
u64 t2 = *reinterpret_cast<u64*>(reinterpret_cast<char*>(t1) + 0x10);
u64 t3 = *reinterpret_cast<u64*>(reinterpret_cast<char*>(t2) + 0x20);
```

`ptr::operator>>` compiles to the same assembly as the raw C version — zero overhead.

---

### Walk (Byte Chase, No Stride)

```cpp
// stx:  explicit intent
auto p = base.walk(off_s{8});
```

```c
// C:  identical assembly
u64 p = *(u64*)((char*)addr + 8);
```

`walk` communicates "read a pointer at this raw byte offset" — its intent is clearer than the C version and impossible to confuse with stride-scaled access.

---

### `p[off].read<T>()` vs `memcpy`

```cpp
// stx:  safe, typed, unaligned-friendly
stx::u32 val = p[off_s{0x200}].read<stx::u32>();
```

```c
// C:  caller must manage alignment and casting manually
u32 val;
memcpy(&val, (char*)addr + 0x200, sizeof(val));
```

```cpp
// C++ raw:  same cost, more ceremony
u32 val;
std::memcpy(&val, reinterpret_cast<const char*>(addr) + 0x200, sizeof(val));
```

All three produce identical `memcpy` calls. `ptr::read` saves the boilerplate and enforces `binary_readable` at compile time. The offset is factored out into `operator[]`, keeping read/write focused on data access.

---

### Stride Management (`step<N>`)

```cpp
// stx:  stride is part of the type
auto entry_table = base.step<4>();   // ptr<void, 4>
uptr entry = (entry_table >> 3 >> 5).addr();
```

```c
// C:  stride math is manual and repeated
u64 e1 = *(u64*)((char*)addr + 3 * 4);
u64 e2 = *(u64*)((char*)e1 + 5 * 4);
```

With `ptr`, the stride is set once via `step<N>()` and `>>` automatically scales — no mental recalculation of `sizeof` at each step.

---

### Arithmetic (`+`, `-`, `++`, `--`)

```cpp
// stx:  byte-level, type-preserving
auto next = base + off_s{0x100};
next += 8;
--next;
```

```c
// C:  same byte-level math, no type safety
void* next = (char*)addr + 0x100;
next = (char*)next + 8;
next = (char*)next - 1;
```

`ptr` arithmetic works at byte granularity — consistent with `walk()` and `operator[]`. No implicit element-type scaling.

---

### Read Pointer (`read_p`)

```cpp
// stx:  read_p<T> returns ptr<T>, chainable
auto chain = base[off_s{0x10}].read_p<Entry>() >> 4;
```

```c
// C:  must re-wrap the result manually
Entry* ptr = *(Entry**)((char*)addr + 0x10);
u64 val = *(u64*)((char*)ptr + 4 * sizeof(void*));
```

`read_p` preserves the `ptr` wrapper, enabling further chaining without manual `reinterpret_cast`.

