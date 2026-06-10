# mem.hpp

## `ptr<T, Stride>`

A non-owning pointer wrapper whose arithmetic operates in byte units.
`ptr<T, Stride = 1>` stores a `uptr` internally — `p + n` moves
`n * sizeof(T) * Stride` bytes forward.

### Template Parameters

| Parameter | Constraint | Default | Description |
|-----------|------------|---------|-------------|
| `T` | Any type (including `void`) | — | Referenced type |
| `Stride` | `uptr` | `1` | Element stride multiplier |

### Construction

```cpp
ptr() noexcept;
ptr(std::nullptr_t) noexcept;
ptr(null_t) noexcept;
ptr(T* p) noexcept;
ptr(const ptr&) noexcept;
ptr(ptr&&) noexcept;
```

```cpp
stx::ptr<int> p1;                  // null
stx::ptr<int> p2{nullptr};         // null
stx::ptr<int> p3{buf};             // from raw pointer
stx::ptr      p4{buf};             // CTAD -> ptr<element_type>
stx::ptr<int, 2> p5{buf};          // stride-2: p+1 advances 8 bytes
```

### Assignment

```cpp
ptr& operator=(const ptr&) noexcept;
ptr& operator=(ptr&&) noexcept;
ptr& operator=(T* p) noexcept;
```

### Dereference

```cpp
T& operator*()  const noexcept;
T* operator->() const noexcept requires (not std::is_void_v<T>);
```

```cpp
*p = 42;
int val = *p;
int x = p->member;
```

For `ptr<void, S>`, `operator*` and `operator->` are disabled.

### Array Access

Raw integral indices use element-level arithmetic (respecting stride).
`off_s`-typed indices use byte-level arithmetic.

```cpp
T& operator[](isize index) const noexcept;    // element index (stride-aware)
T& operator[](off_s offset) const noexcept;   // byte offset
```

```cpp
stx::ptr<stx::u32, 2> p{buf};

p[0] = 10;        // *(buf + 0*4*2) = buf[0]
p[1] = 20;        // *(buf + 1*4*2) = buf[2]

p[stx::off_s{4}] = 30;  // *(buf + 4) = buf[1]  (byte-level)
```

### Arithmetic

| Expression | Effect | Unit |
|------------|--------|------|
| `p + n` | Advance `n` elements | Elements (stride-aware) |
| `p - n` | Rewind `n` elements | Elements |
| `p + off_s{n}` | Advance `n` bytes | Bytes |
| `p - off_s{n}` | Rewind `n` bytes | Bytes |
| `p += n` / `p -= n` | In-place element advance | Elements |
| `p += off_s{n}` / `p -= off_s{n}` | In-place byte advance | Bytes |
| `p - q` | Element difference | Elements |

```cpp
stx::ptr<stx::u32, 1> p{buf};

auto p2 = p + 4;            // +4 elements = +16 bytes
auto p3 = p + stx::off_s{16};  // +16 bytes
isize diff = p3 - p;        // 4 elements

p += 2;                     // advance 2 elements in place
```

### Comparison

```cpp
explicit operator bool() const noexcept;      // non-null check
bool operator==(const ptr&) const noexcept;
bool operator!=(const ptr&) const noexcept;
auto operator<=>(const ptr&) const noexcept;  // three-way
```

```cpp
if (p) {
    // valid pointer
}

if (p < q) {
    // address comparison
}

if (p == stx::null) {
    // null check via null_t
}
```

### Accessors

| Method | Returns |
|--------|---------|
| `get()` | `T*` (raw pointer) |
| `addr()` | `uptr` (address as integer) |

```cpp
T* raw = p.get();
stx::uptr a = p.addr();
```

### Read / Write / Pop

| Method | Behavior |
|--------|----------|
| `read<T>(off)` | Read `T` at byte offset (cursor unchanged) |
| `write<T>(val, off)` | Write `T` at byte offset |
| `pop<T>()` | Read `T` then advance by `sizeof(T)` |
| `read<bounded_array>(off)` | Read array at byte offset |
| `pop<bounded_array>()` | Read array then advance |

```cpp
stx::ptr<stx::u8> p{data};

auto magic = p.read<stx::u32>();           // read u32, no advance
auto type  = p.read<stx::u16>(stx::off_s{4});  // read u16 at byte 4

p.write<stx::u32>(0xDEAD);                 // write u32 at current pos

auto f1 = p.pop<stx::u32>();               // read + advance 4
auto f2 = p.pop<stx::u16>();               // read + advance 2

auto arr = p.pop<stx::u8[12]>();           // read 12 bytes into array
```

This makes sequential binary parsing straightforward:

```cpp
auto read_entry(stx::ptr<std::byte> p) {
    struct Entry { stx::u32 id; stx::u64 timestamp; stx::u32 size; };
    return Entry{
        .id        = p.pop<stx::u32>(),
        .timestamp = p.pop<stx::u64>(),
        .size      = p.pop<stx::u32>(),
    };
}
```

### Type Reinterpretation

```cpp
template<typename U> constexpr ptr<U> as() const noexcept;
```

Reinterprets the stored address as a pointer to `U`. No stride change.

```cpp
auto byte_ptr = int_ptr.as<std::byte>();   // ptr<std::byte>
auto char_ptr = byte_ptr.as<char>();       // ptr<char>
```

### Pointer Alignment

```cpp
template<std::unsigned_integral U = usize>
constexpr ptr align_up(U alignment) const noexcept;

template<std::unsigned_integral U = usize>
constexpr ptr align_down(U alignment) const noexcept;
```

```cpp
stx::ptr<stx::u32> p{reinterpret_cast<stx::u32*>(0x1003)};

auto aligned  = p.align_up(16);    // ptr at 0x1010
auto aligned2 = p.align_down(16);  // ptr at 0x1000
```

### Tagged Access

```cpp
template<typename Tag>
constexpr auto tag() const noexcept;
```

Access address through a `Tag` class exposing a static `addr()` method.
Useful for hardware register maps or struct offset tables.

```cpp
struct UART0 {
    static constexpr stx::uptr addr() { return 0x10000000; }
};

stx::ptr<stx::u32> uart = some_ptr.tag<UART0>();
// equivalent to: ptr at UART0::addr()
```

---

## `mem::read` / `mem::write`

Free-function single-value binary I/O over raw addresses.

```cpp
template<binary_readable T>
T read(const void* src, off_s off = {}) noexcept;

template<binary_readable T>
void write(void* dst, const T& value, off_s off = {}) noexcept;
```

These perform an unaligned-safe `memcpy` from/to the address. They accept
any `address_like` base (pointer, `uptr`, `va_s`).

```cpp
stx::u32 buf[4] = {};

auto v   = stx::mem::read<stx::u32>(buf);          // read at offset 0
auto v2  = stx::mem::read<stx::u64>(buf, stx::off_s{4});  // read at byte 4
auto v3  = stx::mem::read<stx::u32>(stx::va_s{0x1000});    // read from VA

stx::mem::write<stx::u32>(buf, 0xDEADBEEF);        // write at offset 0
stx::mem::write<stx::u16>(buf, static_cast<stx::u16>(0x1234), stx::off_s{8});
```

---

## `mem::read_be` / `mem::write_be`

Big-endian (network byte order) reads and writes. Results are in native
byte order — the swap happens transparently during transfer.

Always operates through the underlying `Raw` type (bypassing any custom
`Type` serialization), then casts back.

```cpp
template<byte_swappable T>
T read_be(const void* src, off_s off = {}) noexcept;

template<byte_swappable T>
void write_be(void* dst, const T& value, off_s off = {}) noexcept;
```

```cpp
// Read big-endian u32 from network packet
auto net = stx::mem::read_be<stx::u32>(packet);       // native-endian result
auto v   = stx::mem::read_be<stx::u16>(packet, stx::off_s{4});

// Write big-endian
stx::mem::write_be<stx::u32>(buf, 0x12345678);

// Works with enums too
enum class Protocol : stx::u16 { TCP = 6, UDP = 17 };
auto p = stx::mem::read_be<Protocol>(packet);          // reads big-endian u16
```

---

## `mem::align_up` / `mem::align_down`

Free-function alignment for unsigned integrals and strong types.

| Function | Effect | Example |
|----------|--------|---------|
| `align_up(v, a)` | Round `v` up to next `a`-aligned boundary | `123 → 128` (align 16) |
| `align_down(v, a)` | Round `v` down to previous `a`-aligned boundary | `123 → 112` (align 16) |

```cpp
template<std::unsigned_integral T>
constexpr T align_up(T value, T alignment) noexcept;

template<std::unsigned_integral T>
constexpr T align_down(T value, T alignment) noexcept;

template<typename T, typename Tag, std::integral U>
constexpr auto align_up(details::strong_type<T, Tag> st, U alignment) noexcept;

template<typename T, typename Tag, std::integral U>
constexpr auto align_down(details::strong_type<T, Tag> st, U alignment) noexcept;
```

```cpp
auto a = stx::mem::align_up(0u, 16u);     // 0
auto b = stx::mem::align_up(1u, 16u);     // 16
auto c = stx::mem::align_up(16u, 16u);    // 16
auto d = stx::mem::align_down(123u, 16u); // 112

// Strong types preserve their domain
auto off = stx::mem::align_up(stx::off_s{123}, 16u);  // off_s{128}
static_assert(std::is_same_v<decltype(off), stx::off_s>);
```

Edge cases:

```cpp
auto zero  = stx::mem::align_up(0u, 1u);    // 0
auto same  = stx::mem::align_up(64u, 64u);  // 64
auto power = stx::mem::align_up(63u, 64u);  // 64
```

---

## `mem::gap_align_v`

Compile-time aligned gap: accumulates `sizeof` each type with padding to
each member's natural alignment, then aligns the total up to `Align`.

```cpp
template<usize Align, typename... Args>
inline constexpr off_s gap_align_v;
```

```cpp
// sizeof(u32)=4, sizeof(u64)=8, sizeof(u16)=2
// u32 at 0, u64 aligns to 8 → padding to 8, u16 at 16
// total = 18, aligned up to 16 → 32
constexpr auto sz = stx::mem::gap_align_v<16, stx::u32, stx::u64, stx::u16>;
// sz == off_s{32}
```

Useful for compile-time struct layout simulation:

```cpp
struct alignas(16) Simulated {
    stx::u32 a;   // offset  0
    stx::u64 b;   // offset  8 (after 4 bytes padding)
    stx::u16 c;   // offset 16
    // padding to 32
};
static_assert(sizeof(Simulated) == stx::mem::gap_align_v<16, stx::u32, stx::u64, stx::u16>.get());
```

