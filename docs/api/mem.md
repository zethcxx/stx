# mem.hpp

## `ptr<T>`

A non-owning pointer wrapper for typed memory access.
Internally stores a `uptr` for arithmetic and dereference.

| Parameter | Constraint | Default | Description |
|-----------|------------|---------|-------------|
| `T` | Any (including `void`) | — | Referenced type |

### Construction

```cpp
ptr() noexcept;                              // null
ptr(T* raw_ptr) noexcept;                    // from raw pointer
explicit ptr(address_like auto addr) noexcept; // from uptr, va_s, etc.
ptr(null_t) noexcept;                        // null
```

```cpp
stx::ptr<int> p1;            // null
stx::ptr<int> p2{buf};       // from T*
stx::ptr      p3{buf};       // CTAD -> ptr<element_type>
stx::ptr<int> p5{stx::null}; // null
stx::ptr<int> p6{stx::uptr(0x1000)}; // from address
```

### Assignment

```cpp
ptr& operator=(T* raw_ptr) noexcept;
ptr& operator=(address_like auto addr) noexcept;
```

```cpp
p = buf;          // raw pointer
p = stx::null;    // null
```

### Accessors

| Method | Returns | Description |
|--------|---------|-------------|
| `raw()` | `T*` | Raw pointer (mutable overload) |
| `raw()` const | `const T*` | Raw pointer (const overload) |
| `addr()` | `uptr` | Address as integer |
| `operator uptr()` | `uptr` | Implicit conversion |
| `operator bool()` | `bool` | Non-null check |

```cpp
T* r = p.raw();
stx::uptr a = p.addr();
if (p) { /* valid */ }
```

### Comparison

```cpp
auto operator<=>(const ptr&) const noexcept = default;
bool operator==(const ptr&) const noexcept = default;
bool operator==(null_t) const noexcept;
bool operator!=(null_t) const noexcept;
```

```cpp
if (p == q) {}
if (p < q)  {}
if (p == stx::null) {}
```

### Dereference

```cpp
T& operator*() noexcept            requires (!std::is_void_v<T>);
const T& operator*() const noexcept requires (!std::is_void_v<T>);
T* operator->() noexcept            requires (!std::is_void_v<T>);
const T* operator->() const noexcept  requires (!std::is_void_v<T>);
```

```cpp
*p = 42;
int v = *p;
p->member = 10;
```

Disabled for `ptr<void>`.

### Operator[] — Displacement (returns `ptr_light`)

Returns a `ptr_light<T>` at the calculated address — this is a
**displacement**, NOT a dereference. `ptr_light` inherits `ptr` but has
`operator[]` deleted (no chaining).

Three overloads control the addressing mode:

| Expression | Step | Formula |
|------------|------|---------|
| `p[n]` — single integral | `sizeof(T)` (element-level) | `addr + n * sizeof(T)` |
| `p[n, s]` — two integrals | `s` (custom byte step) | `addr + n * s` |
| `p[off_s{n}]` — byte_offset | 1 (byte-level) | `addr + n` |

```cpp
stx::ptr<stx::u32> p{buf};

auto el = p[2];              // ptr_light<u32> at address + 2 * sizeof(u32)
auto cs = p[2, 1];           // ptr_light<u32> at address + 2 * 1 (byte-level)
auto by = p[stx::off_s{4}];  // ptr_light<u32> at address + 4

// void pointers: no sizeof multiplier, single integral = byte-level
stx::ptr<void> pv{buf};
auto el4 = pv[2];            // ptr_light<void> at address + 2

// ptr_light usage:
el.write<stx::u32>(42);
auto v = el.read<stx::u32>();
// el[0];  // error: operator[] deleted on ptr_light
```

### Arithmetic

All arithmetic is in **bytes** — only `off_s`/`rva_s` operands are accepted
(no raw integral arithmetic).

| Expression | Effect | Returns |
|------------|--------|---------|
| `p + off_s{n}` | Advance `n` bytes | `ptr` |
| `p - off_s{n}` | Rewind `n` bytes | `ptr` |
| `p += off_s{n}` / `p -= off_s{n}` | In-place | `ptr&` |
| `p - q` | Byte difference | `off_s` |
| `p.diff(addr)` | Diff from any address | `off_s` |

```cpp
auto p2 = p + stx::off_s{16};      // ptr at address + 16
auto p3 = p - stx::off_s{8};       // ptr at address - 8
p += stx::off_s{4};                // advance 4 bytes in place
stx::off_s delta = p3 - p;         // off_s{8}
stx::off_s d2 = p.diff(&x);        // difference from raw pointer
```

### Increment / Decrement

Element-level: advances by `sizeof(T)` (or 1 for `void`).

```cpp
ptr& operator++() noexcept;    // advance by sizeof(T) (or 1 if void)
ptr  operator++(int) noexcept;
ptr& operator--() noexcept;
ptr  operator--(int) noexcept;
```

```cpp
++p;                            // advance 1 element
auto old = p++;                 // post-increment
--p;                            // back 1 element
```

### Safe Read (memcpy, unaligned-safe)

```cpp
template<binary_readable U> U                  read()  const noexcept;
template<bounded_array U>   auto                read()  const noexcept;
template<binary_readable U> U                  pop()   noexcept;        // read + advance
template<bounded_array U>   auto                pop()   noexcept;
template<binary_readable U> void               write(U value) const noexcept;
template<contiguous_buffer R> void             write(R&& range) const noexcept;
template<binary_readable U> ptr&               push(const U& value) noexcept;  // write + advance
template<contiguous_buffer R> ptr&             push(R&& range) noexcept;
```

```cpp
// Read without advance
auto magic = p.read<stx::u32>();
auto arr   = p.read<stx::u8[16]>();

// Pop (read + advance, sequential parse)
auto field1 = p.pop<stx::u32>();     // advance 4
auto field2 = p.pop<stx::u16>();     // advance 2
auto bytes  = p.pop<stx::u8[12]>();  // advance 12

// Write
p.write<stx::u32>(0xDEADBEEF);
p.write(std::vector{stx::u8{1}, stx::u8{2}});  // contiguous_buffer

// Push (write + advance)
p.push(42).push(3.14f);              // chainable, returns ptr&
```

### Read Pointer from Memory

```cpp
template<binary_readable U> ptr<U> read_p() const noexcept;
```

Reads a `uptr` from memory and wraps it in a new `ptr<U>`. Useful for
following pointers stored in binary structures.

```cpp
// A struct at p contains a pointer at offset 8
auto child = p.read_p<int>();              // reads uptr from *p, returns ptr<int>
auto child = (p + stx::off_s{8}).read_p<int>();  // at offset 8
```

### Pointer Chase (memcpy-safe)

Reads `sizeof(ReturnType)` bytes from `address + offset` (byte-level),
then wraps the value in a new `ptr<ReturnType>`.

```cpp
template<typename ReturnType = T, byte_offset OffT> ptr<ReturnType> walk(OffT offset) const noexcept;
template<typename ReturnType = T, std::integral U> auto walk(U offset) const noexcept;
```

```cpp
// p+8 contains a pointer to the next node (stored as uptr)
auto next = p.walk<int>(stx::off_s{8});  // read uptr at p+8, wrap as ptr<int>

// Integral offset is converted to off_s internally (still byte-level)
auto next2 = p.walk<int>(8);             // same as above
```

### Operator>> — Chain / Pointer Chase

Reads a `uptr` from `address + offset` (byte-level, always), then wraps
the value in a new `ptr<T>`. Integral offsets are converted to `off_s`.

```cpp
template<byte_offset OffT> ptr<T> operator>>(OffT offset) const noexcept;
template<std::integral U>  auto operator>>(U offset) const noexcept;
```

```cpp
// Follow a pointer stored at byte offset 8 from p
auto child = p >> stx::off_s{8};

// Integral offset is byte-level (wraps to off_s internally)
auto child2 = p >> 8;   // reads uptr at address + 8 (same as off_s{8})

// Chaining
auto val = (p >> 8 >> 4).read<stx::u32>();  // follow ptr at +8, then ptr at +4
```

### Read Into / Pop Into

```cpp
template<writable_buffer R> void    read_into(R&& buf) const noexcept;
template<writable_buffer R> ptr&    pop_into(R&& buf) noexcept;
```

Reads bytes into a writable container (vector, array, span).

```cpp
std::vector<stx::u8> tmp(64);
p.read_into(tmp);         // copy 64 bytes into tmp, no advance
p.pop_into(tmp);          // copy 64 bytes into tmp, advance 64
```

### Zero-Copy View

```cpp
template<bounded_array U> std::span<const std::remove_all_extents_t<U>> as_view() const noexcept;
template<binary_readable U> std::span<const U> as_view(usize count) const noexcept;
```

Returns a `span` over the memory at the pointer's address.

```cpp
auto s1 = p.as_view<stx::u32[16]>();         // span of 16 u32s
auto s2 = p.as_view<stx::u32>(32);           // span of 32 u32s
```

### Unsafe Read/Write (direct deref)

```cpp
template<binary_readable U> U    read_raw() const noexcept;
template<binary_readable U> void write_raw(U value) const noexcept;
```

Direct dereference — requires alignment, violates strict-aliasing.

```cpp
auto v = p.read_raw<stx::u32>();  // *reinterpret_cast<u32*>(addr)
p.write_raw(42);                  // *reinterpret_cast<u32*>(addr) = 42
```

### Endian-Aware

```cpp
template<std::integral U> U    read_le() const noexcept;
template<std::integral U> U    read_be() const noexcept;
template<byte_swappable U> void write_le(U value) const noexcept;
template<std::integral U> void write_be(U value) const noexcept;
```

```cpp
auto le = p.read_le<stx::u32>();  // little-endian read
auto be = p.read_be<stx::u32>();  // big-endian read (byteswapped)
p.write_be<stx::u32>(0x1234);     // write in big-endian
```

### Type Reinterpretation

```cpp
template<typename U> constexpr ptr<U> as_p() const noexcept;
template<typename U> constexpr auto   as() const noexcept -> U;
```

```cpp
auto byte_ptr = p.as_p<std::byte>();   // ptr<std::byte>, same address
auto as_u64 = p.as<stx::u64>();        // scast<u64>(addr)
```

### Pointer Alignment

```cpp
template<std::unsigned_integral U = usize> constexpr ptr align_up(U alignment) const noexcept;
template<std::unsigned_integral U = usize> constexpr ptr align_down(U alignment) const noexcept;
```

```cpp
auto aligned  = p.align_up(16);    // round address up to 16-byte boundary
auto aligned2 = p.align_down(16);  // round address down
```

### Alignment Check

```cpp
template<typename U> constexpr bool is_aligned() const noexcept;
template<usize Alignment> constexpr bool is_aligned() const noexcept;
```

```cpp
bool ok = p.is_aligned<stx::u32>();   // check alignment for u32
bool ok = p.is_aligned<16>();         // check 16-byte alignment
```

### Function Call

```cpp
template<class Sig, class... Args> decltype(auto) call(Args&&... args) const;
template<class Sig> auto caller() const noexcept;
```

Treats the pointer as a function pointer and invokes it.

```cpp
using Sig = int(float, char);
auto result = p.call<Sig>(3.14f, 'x');
auto fn = p.caller<Sig>();
```

### Swap

```cpp
void swap(ptr& other) noexcept;
friend void swap(ptr& a, ptr& b) noexcept;
```

---

## `ptr_light<T>`

A lightweight proxy returned by `ptr::operator[]`. Inherits `ptr` but
deletes `operator[]` to prevent double-indexing.

```cpp
template<typename T>
struct ptr_light : ptr<T>
{
    using ptr<T>::ptr;
    template<typename U> ptr_light operator[](U) const = delete;
};
```

All `ptr` operations work: `*`, `->`, `read`, `pop`, `write`, `push`,
`read_p`, `walk`, `>>`, `as_p`, etc. Only `[]` is blocked.

```cpp
auto el = p[3];                    // ptr_light<int>
auto v  = el.read<stx::u32>();     // OK
el.write(42);                      // OK
// el[0];                          // error: operator[] deleted
// p[3][4];                        // error: no chaining
```

---

## `mem::read` / `mem::write`

Free-function binary-safe memcpy I/O over any `address_like` base.

```cpp
template<binary_readable Type, address_like Addr, byte_offset OffT = off_s>
Type read(Addr base, OffT off = {}) noexcept;
```

```cpp
// single value — Type deducido del argumento o explícito
template<binary_readable Type, address_like Addr>
void write(Addr base, const Type& value) noexcept;

// range — cualquier contiguous_buffer (span, vector, array, string_view...)
template<address_like Addr, contiguous_buffer R>
void write(Addr base, R&& range) noexcept;
```

```cpp
// read — con o sin offset
stx::u32 buf[4] = {};
auto v  = stx::mem::read<stx::u32>(buf);              // offset 0
auto v2 = stx::mem::read<stx::u64>(buf, stx::off_s{4}); // byte 4
auto v3 = stx::mem::read<stx::u32>(stx::va_s{0x1000});  // VA

// write single — tipo deducido
stx::mem::write(buf, 0xDEADBEEF);                     // deduce u32
stx::mem::write<u64>(buf, 0xDEAD);                    // explícito u64 → 0x000000000000DEAD

// write range
stx::mem::write(buf, std::span{data, len});            // span
stx::mem::write(buf, std::array<u32, 4>{1,2,3,4});     // bounded array
stx::mem::write(buf, "header"sv);                      // string_view
```

## `mem::read_raw` / `mem::write_raw`

Unsafe direct-deref versions (requires alignment).

```cpp
template<binary_readable Type, address_like Addr>
Type read_raw(Addr base) noexcept;

template<binary_readable Type, address_like Addr>
void write_raw(Addr base, Type value) noexcept;
```

```cpp
auto v = stx::mem::read_raw<u32>(buf);   // *reinterpret_cast<u32*>(addr)
stx::mem::write_raw(buf, 42);             // *reinterpret_cast<int*>(addr) = 42
```

## `mem::read_le` / `mem::write_le`

Little-endian (passthrough, same as `read`/`write`).

```cpp
template<byte_swappable Type, address_like Addr, byte_offset OffT = off_s>
Type read_le(Addr base, OffT off) noexcept;
// read_le(base) overload without offset

template<byte_swappable Type, address_like Addr, byte_offset OffT = off_s>
void write_le(Addr base, OffT off, Type value) noexcept;
// write_le(base, value) overload without offset
```

## `mem::read_be` / `mem::write_be`

Big-endian (byteswaps on little-endian hosts). Always goes through the
underlying `Raw` type, then casts back — useful for enums.

```cpp
template<byte_swappable Type, address_like Addr, byte_offset OffT = off_s>
Type read_be(Addr base, OffT off) noexcept;

template<byte_swappable Type, address_like Addr, byte_offset OffT = off_s>
void write_be(Addr base, OffT off, Type value) noexcept;
```

```cpp
// Network packet parsing
auto net = stx::mem::read_be<stx::u32>(packet);          // big-endian u32
auto v   = stx::mem::read_be<stx::u16>(packet, stx::off_s{4});

// Write big-endian
stx::mem::write_be(buf, 0x12345678);

// Enums work too
enum class Proto : stx::u16 { TCP = 6, UDP = 17 };
auto p = stx::mem::read_be<Proto>(packet);               // through underlying type
```

---

## `mem::align_up` / `mem::align_down`

| Function | Effect | Example |
|----------|--------|---------|
| `align_up(v, a)` | Round up to `a` boundary | `1 → 16` (align 16) |
| `align_down(v, a)` | Round down to `a` boundary | `17 → 16` (align 16) |

```cpp
template<std::unsigned_integral T> constexpr T align_up(T value, T alignment) noexcept;
template<std::unsigned_integral T> constexpr T align_down(T value, T alignment) noexcept;
template<typename T, typename Tag, std::integral U> constexpr auto align_up(details::strong_type<T, Tag> st, U alignment) noexcept;
template<typename T, typename Tag, std::integral U> constexpr auto align_down(details::strong_type<T, Tag> st, U alignment) noexcept;
```

```cpp
auto a = stx::mem::align_up(1u, 16u);      // 16
auto d = stx::mem::align_down(123u, 16u);  // 112

// Strong types preserved
auto off = stx::mem::align_up(stx::off_s{123}, 16u);  // off_s{128}
```

---

## `mem::gap_v`

Sum of `sizeof` each type in the pack.

```cpp
template<typename... Args>
inline constexpr off_s gap_v;
```

```cpp
constexpr auto sz = stx::mem::gap_v<stx::u32, stx::u64, stx::u16>;  // off_s{14}
```

---

## `mem::gap_align_v`

Aligned compile-time gap: accumulates `sizeof` with natural-alignment
padding, then aligns total up to `Align`.

```cpp
template<usize Align, typename... Args>
inline constexpr off_s gap_align_v;
```

```cpp
constexpr auto sz = stx::mem::gap_align_v<16, stx::u32, stx::u64, stx::u16>;
// u32 at 0, u64 at 8 (padded), u16 at 16, total=18, aligned=32
// sz == off_s{32}
```
