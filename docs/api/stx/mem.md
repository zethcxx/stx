# mem.hpp

All examples assume `using namespace stx;` for brevity.

## `ptr<T>`

A non-owning pointer wrapper for typed memory access.
Internally stores a `uptr` for arithmetic and dereference.

| Parameter | Constraint | Default | Description |
|-----------|------------|---------|-------------|
| `T` | Any (including `void`) | — | Referenced type |

### Construction (stx::ptr)

```cpp
ptr() noexcept;             // null
ptr( T* raw_ptr ) noexcept; // from raw pointer
ptr( null_t     ) noexcept; // null
explicit ptr( address_like auto addr ) noexcept; // from uptr, va_s, etc.
```

```cpp
ptr<int> p1;         // null
ptr<int> p2{ buf  }; // from T*
ptr      p3{ buf  }; // CTAD -> ptr<element_type>
ptr<int> p5{ null }; // null
ptr<int> p6{ uptr( 0x1000 ) }; // from address
```

### Assignment (stx::ptr)

```cpp
ptr& operator=( T* raw_ptr             ) noexcept;
ptr& operator=( address_like auto addr ) noexcept;
```

```cpp
p = buf ; // raw pointer
p = null; // null
```

### Accessors (stx::ptr)

| Method | Returns | Description |
|--------|---------|-------------|
| `raw()` | `T*` | Raw pointer (mutable overload) |
| `raw()` const | `const T*` | Raw pointer (const overload) |
| `addr()` | `uptr` | Address as integer |
| `operator uptr()` | `uptr` | Implicit conversion |
| `operator bool()` | `bool` | Non-null check |

```cpp
auto r = p.raw (); // T*
auto a = p.addr(); // uptr
if ( p ) { /* valid */ }
```

### Comparison (stx::ptr)

```cpp
auto operator<=>( const ptr& ) const noexcept = default;
bool operator== ( const ptr& ) const noexcept = default;
bool operator== ( null_t     ) const noexcept;
bool operator!= ( null_t     ) const noexcept;
```

```cpp
if ( p == q    ) {}
if ( p <  q    ) {}
if ( p == null ) {}
```

### Dereference (stx::ptr)

```cpp
const T& operator* () const noexcept requires (!std::is_void_v<T>);
      T& operator* ()       noexcept requires (!std::is_void_v<T>);

const T* operator->() const noexcept requires (!std::is_void_v<T>);
      T* operator->()       noexcept requires (!std::is_void_v<T>);
```

```cpp
*p = 42;
auto v = *p; // int
p->member = 10;
```

Disabled for `ptr<void>`.

### Operator[] — Displacement (stx::ptr)

Returns a `ptr<T>` at the calculated address — this is a **displacement**,
NOT a dereference. The `&&` (temporary) overload is deleted, so
`p[n][m]` is rejected at compile time:

```cpp
auto el = p[2]; // OK: lvalue [] → ptr<T>
// p[2][3];     // error: [] on temporary is deleted
el[1];          // OK: el is an lvalue
```

Two overloads control the addressing mode:

| Expression | Step | Formula |
|------------|------|---------|
| `p[n]` — single integral | `sizeof(T)` (element-level) | `addr + n * sizeof(T)` |
| `p[n, s]` — two integrals | `s` (custom byte step) | `addr + n * s` |

For byte-level displacement, `p + off_s{n}` is the arithmetic alternative
to `p[n, 1]` — both give `addr + n`:

| Form           | Returns  | Formula               |
|----------------|----------|-----------------------|
| `p[n, 1]`      | `ptr<T>` | `addr + n` (via `[]`) |
| `p + off_s{n}` | `ptr<T>` | `addr + n` (via `+` ) |

```cpp
ptr<u32> base{buf};

auto x = base[2   ];      // ptr<u32> at address + 2 * sizeof(u32)
auto y = base[2, 1];      // ptr<u32> at address + 2 (byte-level)
auto z = base + off_s{2}; // ptr<u32> at address + 2 (byte-level)

// void pointers: no sizeof multiplier, single integral = byte-level
ptr<void> void_ptr{ buf };
auto a = vodi_ptr[2];     // ptr<void> at address + 2

// The returned ptr supports all operations:
a.write<u32>(42);
auto v = el.read<u32>();
el[1];                       // lvalue [] is fine
```

### Arithmetic (stx::ptr + off_s)

All arithmetic is in **bytes** — only `off_s`/`rva_s` operands are accepted
(no raw integral arithmetic).

| Expression                        | Effect                | Returns |
|-----------------------------------|-----------------------|---------|
| `p + off_s{n}`                    | Advance `n` bytes     | `ptr`   |
| `p - off_s{n}`                    | Rewind `n` bytes      | `ptr`   |
| `p += off_s{n}` / `p -= off_s{n}` | In-place              | `ptr&`  |
| `p - q`                           | Byte difference       | `off_s` |
| `p.diff(addr)`                    | Diff from any address | `off_s` |

```cpp
auto p1 = p + off_s{ 16 }; // ptr at address + 16
auto p2 = p - off_s{ 8  }; // ptr at address - 8

p += off_s{ 4 };           // advance 4 bytes in place

auto delta = p2 - p;      // off_s{8}
auto d = p.diff(&x);      // off_s, difference from raw pointer

// gap_v computes compile-time byte gaps — use it directly as an offset:
auto at_gap = p + mem::gap_v<u32, u64>;  // skip u32+u64 = 12 bytes

// Combined with read/write:
auto p3 = (p + mem::gap_v<int[4]>).read_raw<int>(); // read at byte offset sizeof(int)*4
(p + mem::gap_v<Header, u32>).write(value);   // write after Header + u32
```

### Increment / Decrement (stx::ptr)

Element-level: advances by `sizeof(T)` (or 1 for `void`).

```cpp
ptr& operator++(void) noexcept; // advance by sizeof(T) (or 1 if void)
ptr  operator++(int ) noexcept;
ptr& operator--(void) noexcept;
ptr  operator--(int ) noexcept;
```

```cpp
++p;             // advance 1 element
auto old = p++;  // post-increment
--p;             // back 1 element
```

### Safe Read (stx::ptr)

```cpp
template<binary_readable   U> U     read() const noexcept;
template<bounded_array     U> auto  read() const noexcept;
template<binary_readable   U> U     pop ()       noexcept; // read + advance
template<bounded_array     U> auto  pop ()       noexcept;

template<binary_readable   U> void  write (       U   value ) const noexcept;
template<contiguous_buffer R> void  write (       R&& range ) const noexcept;
template<binary_readable   U> ptr&  push  ( const U&  value )       noexcept;  // write + advance
template<contiguous_buffer R> ptr&  push  (       R&& range )       noexcept;
```

```cpp
// Read without advance
auto magic = p.read<u32>();
auto arr   = p.read<u8[16]>();

// Pop (read + advance, sequential parse)
auto field1 = p.pop<u32>()   ;  // advance 4
auto field2 = p.pop<u16>()   ;  // advance 2
auto bytes  = p.pop<u8[12]>();  // advance 12

// Write
p.write<u32>(0xDEADBEEF);
p.write(std::vector{u8{1}, u8{2}});  // contiguous_buffer

// Push (write + advance)
p.push(42).push(3.14f); // chainable, returns ptr&

// Read/write at an offset — use pointer arithmetic:
auto v = (p + off_s{8}).read<u32>();
(p + mem::gap_v<Header>).write(value);
```

### Read Pointer from Memory (stx::ptr)

```cpp
template<binary_readable U> ptr<U> read_p() const noexcept;
```

Reads a `uptr` from memory and wraps it in a new `ptr<U>`. Useful for
following pointers stored in binary structures.

```cpp
// A struct at p contains a pointer at offset 8
auto child = p.read_p<int>();              // reads uptr from *p, returns ptr<int>
auto child = (p + off_s{8}).read_p<int>(); // at offset 8
// or: auto child = p[8, 1].read_p<int>();
```

### Pointer Chase (stx::ptr)

Reads `sizeof(ReturnType)` bytes from `address + offset` (byte-level),
then wraps the value in a new `ptr<ReturnType>`.

```cpp
template<typename ReturnType = T, byte_offset   OffT> auto walk( OffT offset ) const noexcept -> ptr<ReturnType>;
template<typename ReturnType = T, std::integral U   > auto walk( U    offset ) const noexcept -> ptr<ReturnType>;
```

```cpp
// p+8 contains a pointer to the next node (stored as uptr)
auto next = p.walk<int>(off_s{8});  // read uptr at p+8, wrap as ptr<int>

// Integral offset is converted to off_s internally (still byte-level)
auto next2 = p.walk<int>(8);             // same as above
```

### Operator>> — Chain / Pointer Chase (stx::ptr)

Reads a `uptr` from `address + offset` (byte-level, always), then wraps
the value in a new `ptr<T>`. Integral offsets are converted to `off_s`.

```cpp
template<byte_offset OffT> ptr<T> operator>>(OffT offset) const noexcept;
template<std::integral U>  auto operator>>(U offset) const noexcept;
```

```cpp
// Follow a pointer stored at byte offset 8 from p
auto child = p >> off_s{8};

// Integral offset is byte-level (wraps to off_s internally)
auto child2 = p >> 8;   // reads uptr at address + 8 (same as off_s{8})

// Chaining
auto val = (p >> 8 >> 4).read<u32>();  // follow ptr at +8, then ptr at +4
```

### Read Into / Pop Into (stx::ptr)

```cpp
template<writable_buffer R> void read_into(R&& buf) const noexcept;
template<writable_buffer R> ptr& pop_into (R&& buf)       noexcept;
```

Reads bytes into a writable container (vector, array, span).

```cpp
std::vector<u8> tmp(64);

p.read_into(tmp);  // copy 64 bytes into tmp, no advance
p.pop_into (tmp);  // copy 64 bytes into tmp, advance 64
```

### Zero-Copy View (stx::ptr)

```cpp
template<bounded_array   U> std::span<const std::remove_all_extents_t<U>> as_view()            const noexcept;
template<binary_readable U> std::span<const U>                            as_view(usize count) const noexcept;
```

Returns a `span` over the memory at the pointer's address.

```cpp
auto s1 = p.as_view<u32[16]>();  // span of 16 u32s (compile-time)
auto s2 = p.as_view<u32>(32);    // span of 32 u32s (runtime)
```

### Unsafe Read/Write (stx::ptr, direct deref)

```cpp
template<binary_readable U> U    read_raw()         const noexcept;
template<binary_readable U> void write_raw(U value) const noexcept;
```

Direct dereference — requires alignment, violates strict-aliasing.

```cpp
auto v = p.read_raw<u32>();  // *reinterpret_cast<u32*>(addr)
p.write_raw(42);             // *reinterpret_cast<u32*>(addr) = 42
```

### Endian-Aware (stx::ptr)

```cpp
template<std::integral  U> U    read_le() const noexcept;
template<std::integral  U> U    read_be() const noexcept;

template<byte_swappable U> void write_le(U value) const noexcept;
template<std::integral  U> void write_be(U value) const noexcept;
```

```cpp
auto le = p.read_le<u32>();  // little-endian read
auto be = p.read_be<u32>();  // big-endian read (byteswapped)
p.write_be<u32>(0x1234);     // write in big-endian
```

### Type Reinterpretation (stx::ptr)

```cpp
template<typename U> constexpr ptr<U> as_p() const noexcept;
template<typename U> constexpr auto   as  () const noexcept -> U;
```

```cpp
auto byte_ptr = p.as_p<std::byte>();   // ptr<std::byte>, same address
auto as_u64   = p.as<u64>();           // scast<u64>(p.addr())
```

### Pointer Alignment (stx::ptr)

```cpp
template<std::unsigned_integral U = usize> constexpr ptr align_up  (U alignment) const noexcept;
template<std::unsigned_integral U = usize> constexpr ptr align_down(U alignment) const noexcept;
```

```cpp
auto aligned  = p.align_up  (16);  // round address up to 16-byte boundary
auto aligned2 = p.align_down(16);  // round address down
```

### Alignment Check (stx::ptr)

```cpp
template<typename U>      constexpr bool is_aligned() const noexcept;
template<usize Alignment> constexpr bool is_aligned() const noexcept;
```

```cpp
bool ok = p.is_aligned<u32>();   // check alignment for u32
bool ok = p.is_aligned<16>();    // check 16-byte alignment
```

### Function Call (stx::ptr)

```cpp
template<class Sig, class... Args> decltype(auto) call(Args&&... args) const;
template<class Sig> auto caller() const noexcept;
```

Treats the pointer as a function pointer and invokes it.

```cpp
using Sig = int(float, char);

auto result = p.call<Sig>(3.14f, 'x'); // invoke p as a functor
auto fn     = p.caller<Sig>();         // cast p to a functor
```

### Swap (stx::ptr)

```cpp
void swap(ptr& other) noexcept;
friend void swap(ptr& a, ptr& b) noexcept;
```

---

## `mem::read` / `mem::write`

Free-function binary-safe memcpy I/O over any `address_like` base or `writable_buffer`.

```cpp
template<binary_readable Type, address_like Addr>
Type read(Addr base) noexcept;
```

```cpp
// --- address_like base (raw pointer / VA / ptr<T>) -----------------------

// single value — Type is automatically deduced
template<binary_readable Type, address_like Addr>
    requires (not contiguous_buffer<Type>)
void write(Addr base, const Type& value) noexcept;

template<binary_readable Type, address_like Addr>
    requires (not contiguous_buffer<Type>)
void write(Addr base, off_s offset, const Type& value) noexcept;

// range — any contiguous_buffer (span, vector, array, string_view...)
template<address_like Addr, contiguous_buffer R>
void write(Addr base, R&& range) noexcept;

template<address_like Addr, contiguous_buffer R>
void write(Addr base, off_s offset, const R& range) noexcept;

// --- writable_buffer (span, vector, array...) ---------------------------

template<writable_buffer Dest, binary_readable Type>
    requires (not contiguous_buffer<Type>)
void write(Dest&& dest, off_s offset, const Type& value) noexcept;

template<writable_buffer Dest, binary_readable Type>
    requires (not contiguous_buffer<Type>)
void write(Dest&& dest, const Type& value) noexcept;

template<writable_buffer Dest, contiguous_buffer R>
void write(Dest&& dest, off_s offset, const R& range) noexcept;

template<writable_buffer Dest, contiguous_buffer R>
void write(Dest&& dest, const R& range) noexcept;
```

```cpp
using namespace stx;

u32 buf[4] = {};
auto v   = mem::read<u32>(buf);                      // offset 0
auto v2  = mem::read<u64>(buf + off_s{4});           // byte 4 (ptr arithmetic)
auto v3  = mem::read<u32>(va_s{0x1000});             // VA
auto v4  = mem::read<u16>(buf + mem::gap_v<u32>);   // skip u32

// write to address_like base — offset overload or pointer arithmetic
mem::write(buf, 0xDEADBEEF);                          // deduce u32, offset 0
mem::write(buf, off_s{8}, 0xDEAD);                    // deduce u16, byte 8 (offset overload)
mem::write<u64>(va_s{0x1000} + off_s{4}, 0xDEAD);    // VA + offset (ptr arithmetic)

// write range — offset overload
mem::write(buf, off_s{0}, std::span{data, len});      // span at offset 0
mem::write(buf, off_s{4}, std::array<u32, 4>{1,2,3,4}); // bounded array at byte 4
mem::write(buf, off_s{8}, "header"sv);                // string_view at byte 8

// write into writable_buffer (span, vector, string, array...)
mem::write(std::span<std::byte>{buf}, off_s{2}, u32{0x12345678});
mem::write(my_vector, off_s{0}, "data");

// gap_v for compile-time offsets:
mem::write(buf + mem::gap_v<u32, u16>, 0x42);         // skip u32+u16, write u8
```

## `mem::read_raw` / `mem::write_raw`

Unsafe direct-deref versions (requires alignment).

```cpp
template<binary_readable Type, address_like Addr>
Type read_raw( Addr base ) noexcept;

template<binary_readable Type, address_like Addr>
void write_raw( Addr base, Type value ) noexcept;
```

```cpp
auto v = mem::read_raw<u32>(buf); // *reinterpret_cast<u32*>(addr)
mem::write_raw(buf, 42);          // *reinterpret_cast<int*>(addr) = 42
```

## `mem::read_le` / `mem::write_le`

Little-endian (passthrough, same as `read`/`write`).
Offset via pointer arithmetic.

```cpp
template<byte_swappable Type, address_like Addr>
Type read_le( Addr base ) noexcept;

template<byte_swappable Type, address_like Addr>
void write_le( Addr base, Type value ) noexcept;
```

## `mem::read_be` / `mem::write_be`

Big-endian (byteswaps on little-endian hosts). Always goes through the
underlying `Raw` type, then casts back — useful for enums.
Offset via pointer arithmetic.

```cpp
template<byte_swappable Type, address_like Addr>
Type read_be( Addr base ) noexcept;

template<byte_swappable Type, address_like Addr>
void write_be( Addr base, Type value ) noexcept;
```

```cpp
// Network packet parsing
auto net = mem::read_be<u32>(packet);            // big-endian u32
auto v   = mem::read_be<u16>(packet + off_s{4}); // byte 4

// Write big-endian (offset via pointer arithmetic)
mem::write_be(packet + off_s{8}, 0x12345678);

// gap_v works too:
mem::write_be(packet + mem::gap_v<u32>, u16{0x1234});

// Enums work too
enum class Proto : u16 { TCP = 6, UDP = 17 };
auto p = mem::read_be<Proto>(packet);                     // through underlying type
```

---

## `mem::align_up` / `mem::align_down`

| Function           | Effect                     | Example              |
|--------------------|----------------------------|----------------------|
| `align_up(v, a)`   | Round up to `a` boundary   | `1 → 16` (align 16)  |
| `align_down(v, a)` | Round down to `a` boundary | `17 → 16` (align 16) |

```cpp
template<std::unsigned_integral T> constexpr T align_up  (T value, T alignment) noexcept;
template<std::unsigned_integral T> constexpr T align_down(T value, T alignment) noexcept;

template<typename T, typename Tag, std::integral U> constexpr auto align_up  (details::strong_type<T, Tag> st, U alignment) noexcept;
template<typename T, typename Tag, std::integral U> constexpr auto align_down(details::strong_type<T, Tag> st, U alignment) noexcept;
```

```cpp
auto a = mem::align_up  (1u  , 16u);  // 16
auto d = mem::align_down(123u, 16u);  // 112

// Strong types preserved
auto off = mem::align_up(off_s{123}, 16u);  // off_s{128}
```

---

## STL Compatibility (ptr<T>)

### `std::hash<ptr<T>>`

Enables use of `ptr<T>` in unordered associative containers:

```cpp
std::unordered_set<stx::ptr<u32>> seen;
seen.insert(buf.as_p<u32>());
```

Hashes the underlying address (`uptr`).

### `std::formatter<ptr<T>>`

Enables formatting via `std::format` / `std::print`:

```cpp
stx::ptr<u32> p{addr};
fmtprint("ptr at {}\n", p);  // e.g. "ptr at 0x7ffd12345678"
```

Formats the stored address as `void*` (hex prefix + lowercase hex digits).

---

## `mem::gap_v`

Sum of `sizeof` each type in the pack.

```cpp
template<typename... Args>
inline constexpr off_s gap_v;
```

```cpp
constexpr auto sz = mem::gap_v<u32, u64, u16>;  // off_s{14}
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
constexpr auto sz = mem::gap_align_v<16, u32, u64, u16>;
// u32 at 0, u64 at 8 (padded), u16 at 16, total=18, aligned=32
// sz == off_s{32}
```

