# mem.hpp

## `ptr<T, Stride>`

A non-owning pointer wrapper with compile-time stride and arithmetic in bytes.

`ptr<T, Stride = 1>` wraps a `T*` internally but performs all pointer arithmetic in byte units: `p + n` moves `n * sizeof(T) * Stride` bytes.

### Construction

```cpp
template<typename T, usize Stride = 1>
class ptr
{
    ptr() noexcept;
    ptr(std::nullptr_t) noexcept;
    ptr(null_t) noexcept;
    ptr(T* p) noexcept;
    ptr(const ptr&) noexcept;
    ptr(ptr&&) noexcept;
};
```

```cpp
stx::ptr<int> p;               // default (nullptr)
stx::ptr<int> p2{nullptr};     // null
stx::ptr     p3{data};         // CTAD -> ptr<element_type>
stx::ptr<int, 2> p4{buf};      // stride-2 pointer
```

### Assignment

```cpp
ptr& operator=(const ptr&) noexcept;
ptr& operator=(ptr&&) noexcept;
ptr& operator=(T* p) noexcept;
```

### Dereference

```cpp
[[nodiscard]] constexpr T& operator*()  const noexcept;
[[nodiscard]] constexpr T* operator->() const noexcept
    requires (not std::is_void_v<T>);
```

```cpp
*p = 42;
int val = p->member;
```

### Array Access

```cpp
[[nodiscard]] constexpr T& operator[](isize index) const noexcept; // raw integral index
[[nodiscard]] constexpr T& operator[](off_s offset) const noexcept; // byte offset
```

```cpp
auto a = p[2];        // element at index 2 (stride-aware)
auto b = p[off_s{8}]; // element at byte offset 8
```

### Arithmetic

```cpp
[[nodiscard]] constexpr ptr operator+(isize n) const noexcept; // elements (stride-aware)
[[nodiscard]] constexpr ptr operator-(isize n) const noexcept;
[[nodiscard]] constexpr ptr operator+(off_s offset) const noexcept; // bytes
[[nodiscard]] constexpr ptr operator-(off_s offset) const noexcept;
ptr& operator+=(isize n) noexcept;
ptr& operator-=(isize n) noexcept;
ptr& operator+=(off_s offset) noexcept;
ptr& operator-=(off_s offset) noexcept;
[[nodiscard]] constexpr isize operator-(ptr other) const noexcept; // difference in elements
```

```cpp
auto p2 = p + 4;        // advance 4 elements
auto p3 = p + off_s{16}; // advance 16 bytes
isize d = p3 - p;        // element difference
p += 2;                  // in-place advance
```

### Comparison

```cpp
[[nodiscard]] explicit operator bool() const noexcept;
bool operator==(const ptr&) const noexcept;
bool operator!=(const ptr&) const noexcept;
auto operator<=>(const ptr&) const noexcept;
```

```cpp
if (p) { /* valid */ }
if (p < p2) {}
```

### Access

```cpp
[[nodiscard]] constexpr T* get()     const noexcept;
[[nodiscard]] constexpr uptr addr()  const noexcept;
```

```cpp
T* raw = p.get();
uptr a = p.addr();
```

### Read / Write

```cpp
template<binary_readable T>                  T    read(off_s off = {})  const;
template<binary_readable T>                  void write(const T& val, off_s off = {});
template<binary_readable T>                  T    pop();
template<bounded_array T> requires (not std::is_void_v<Element>) auto pop();
template<bounded_array T> requires (not std::is_void_v<Element>) auto read(off_s off = {}) const;
```

```cpp
u32 v = p.read<u32>();      // read u32 at current position
p.write<u32>(42);           // write u32
auto ba = p.pop<u8[4]>();   // pop 4 bytes into array, advance
p.read<u32>(off_s{8});      // read u32 at byte offset 8
```

### Type Reinterpretation

```cpp
template<typename U> [[nodiscard]] constexpr ptr<U> as() const noexcept;
```

```cpp
auto byte_ptr = p.as<std::byte>(); // reinterpret as byte pointer
```

### Tagged Access

```cpp
template<typename Tag>
[[nodiscard]] constexpr auto tag() const noexcept;
```

Access via `Tag` class with static `addr()` method.

---

## `mem::read` / `mem::write`

Free-function binary reads/writes from/to raw pointers.

```cpp
template<binary_readable T> T     read(const void* src, off_s off = {}) noexcept;
template<binary_readable T> void  write(void* dst, const T& value, off_s off = {}) noexcept;
```

```cpp
auto v = stx::mem::read<u32>(data);          // read u32 at data
stx::mem::write<u32>(data, 0xDEAD);         // write u32 to data
stx::mem::read<u32>(data, off_s{4});         // read u32 at offset 4
```

---

## `mem::read_be` / `mem::write_be`

Big-endian (network byte order) reads/writes. Swaps bytes from/to native endianness, always through the underlying `Raw` type (bypassing any custom `Type` serialization).

```cpp
template<byte_swappable T>
[[nodiscard]] T read_be(const void* src, off_s off = {}) noexcept;

template<byte_swappable T>
void write_be(void* dst, const T& value, off_s off = {}) noexcept;
```

```cpp
u32 net = stx::mem::read_be<u32>(data);       // big-endian read
stx::mem::write_be<u32>(data, 0x1234);        // big-endian write
u16 v = stx::mem::read_be<u16>(data, off_s{4}); // at offset 4

---

## `mem::align_up` / `mem::align_down`

Rounds a value up or down to the nearest power-of-two boundary. Works on unsigned integrals and strong types (`off_s`, `rva_s`, `va_s`).

```cpp
template<std::unsigned_integral T>
[[nodiscard]] constexpr T align_up(T value, T alignment) noexcept;

template<std::unsigned_integral T>
[[nodiscard]] constexpr T align_down(T value, T alignment) noexcept;

template<typename T, typename Tag, std::integral U>
[[nodiscard]] constexpr auto align_up(details::strong_type<T, Tag> st, U alignment) noexcept;

template<typename T, typename Tag, std::integral U>
[[nodiscard]] constexpr auto align_down(details::strong_type<T, Tag> st, U alignment) noexcept;
```

```cpp
auto a = stx::mem::align_up(123u, 16u);   // 128
auto d = stx::mem::align_down(123u, 16u); // 112
auto off = stx::mem::align_up(off_s{123}, 16); // off_s{128}
```

---

## `mem::gap_align_v`

Compile-time aligned gap: total `sizeof` with each type aligned to its natural alignment, then the total aligned up to `Align`.

```cpp
template<usize Align, typename... Args>
inline constexpr off_s gap_align_v;
```

```cpp
constexpr auto sz = stx::mem::gap_align_v<16, u32, u64, u16>; // off_s{16}
```
```
