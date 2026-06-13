# fs.hpp

All examples assume `using namespace stx;` for brevity.

## `fs::dirty_vector`

A `std::vector` with an allocator that does NOT zero-initialize elements.
Useful for buffers that will be overwritten entirely.

```cpp
template<binary_readable Type = u8>
using dirty_vector = std::vector<Type, details::vec_init_allocator<Type>>;
```

```cpp
// read returns dirty_vector directly, no wasted zeroing:
auto vec = fs::read<u8>(file, off_s{0}, 1024);

// Or construct manually and overwrite:
auto buf = fs::dirty_vector<u8>(4096);
read(fd, buf.data(), buf.size());  // POSIX read into uninitialized memory
```

## `fs::origin`

Seek-origin enum for `memcur::seek` / `memcur::advance`.

| Enumerator        | Behavior                   |
|-------------------|----------------------------|
| `origin::begin`   | Seek from start            |
| `origin::current` | Seek from current position |
| `origin::end`     | Seek from end              |

```cpp
using fs::origin;
```

---

## `fs::read` / `fs::write` (stream I/O)

Positional read/write from/to `std::istream`/`std::ostream`.

### Single-value read

```cpp
template<binary_readable Type>
std::expected<Type, std::errc> read(std::istream& file, off_s offset = {}, origin dir = origin::begin) noexcept;
```

### Buffer read (span, count, or fixed array)

```cpp
template<binary_readable Type>
auto read(std::istream&, std::span<Type>, off_s, origin = begin) noexcept -> std::expected<void, std::errc>;

template<binary_readable Type>
auto read(std::istream&, off_s, usize count, origin = begin) noexcept -> std::expected<dirty_vector<Type>, std::errc>;

template<binary_readable Type, usize Size>
auto read(std::istream&, off_s = {}, origin = begin) noexcept -> std::expected<std::array<Type, Size>, std::errc> ;
```

### Single-value write

```cpp
template<binary_readable Type>
std::expected<void, std::errc> write(std::ostream&, off_s, const Type& value) noexcept;
```

### Buffer write

```cpp
template<binary_readable Type>
std::expected<void, std::errc> write(std::ostream&, off_s, std::span<const Type>) noexcept;
```

### Navigation helpers

```cpp
void setpos(std::istream&, off_s, origin = begin) noexcept;
void setpos(std::ostream&, off_s, origin = begin) noexcept;

void advance(std::istream&, off_s) noexcept;
void advance(std::ostream&, off_s) noexcept;
```

```cpp
std::ifstream file{"data.bin", std::ios::binary};

// Single value at offset
auto magic = fs::read<u32>(file, off_s{0});
if (magic) {
    // use magic.value()
}

// Read into a buffer
std::vector<u8> buf(1024);
auto ok = fs::read(file, std::span{buf}, off_s{0x100});

// Read fixed array
auto arr = fs::read<u8, 16>(file, off_s{0x200});

// Write
fs::write(file, off_s{0}, 0xDEADBEEF);
```

---

## `map_flag`

Flags controlling `map_file::open` behavior.

| Flag                 | Effect                                         |
|----------------------|------------------------------------------------|
| `map_flag::none`     | Read-only, private, no populate                |
| `map_flag::write`    | Read-write mapping                             |
| `map_flag::exec`     | Executable mapping                             |
| `map_flag::shared`   | Share with other processes (MAP_SHARED)        |
| `map_flag::priv`     | Copy-on-write (MAP_PRIVATE)                    |
| `map_flag::populate` | Pre-populate page tables (MAP_POPULATE, Linux) |

```cpp
map_flag flags = map_flag::write | map_flag::shared;
```

---

## `memcur<ByteType>`

A non-owning cursor over a contiguous range of byte-like elements. Stores
a `ptr<ByteType>` for the base and a `ptr<ByteType>` for the current
position. `ByteType` must satisfy `buffer_type` (`char`, `std::byte`,
`u8`, `i8`).

### State

| Method            | Returns | Description                        |
|-------------------|---------|------------------------------------|
| `base()`          | `uptr`  | Start address of the buffer        |
| `size()`          | `usize` | Total size in elements             |
| `tell()`          | `off_s` | Current byte offset from base      |
| `remaining()`     | `off_s` | Remaining bytes from cursor to end |
| `operator bool()` | `bool`  | Non-null check                     |

```cpp
memcur cur{buf, 256};
cur.advance(off_s{32});

auto pos = cur.tell();        // off_s{32}
auto rem = cur.remaining();   // off_s{224}
```

### Cursor Movement

```cpp
void seek(off_s, origin dir = origin::begin) noexcept;
void advance(off_s) noexcept;
```

```cpp
cur.seek(off_s{0});              // rewind to start
cur.seek(off_s{-16}, origin::end);  // 16 bytes before end
cur.advance(off_s{8});              // advance 8 bytes
```

### Pop (read + advance)

```cpp
template<binary_readable T> T pop() noexcept;
template<bounded_array U> details::bounded_array_t<U> pop() noexcept;
```

```cpp
auto a   = cur.pop<u32>();      // read u32, advance 4
auto b   = cur.pop<u16>();      // read u16, advance 2
auto arr = cur.pop<u8[16]>();   // read 16 bytes, advance 16
```

### Push (write + advance)

```cpp
template<binary_readable   T> auto& push(const T& value) noexcept;
template<contiguous_buffer R> auto& push(R&& buf) noexcept;
```

Returns `*this` for chaining.

```cpp
cur.push(u32{0xDEAD}).push(u16{0xBEEF});
cur.push(std::vector{u8{1, 2, 3, 4}});
```

### Read Into / Pop Into

```cpp
template<writable_buffer R> void   read_into(R&& buf) noexcept;
template<writable_buffer R> auto&  pop_into (R&& buf) noexcept;
```

```cpp
std::array<u8, 32> tmp;
cur.read_into(tmp);      // copy 32 bytes, no advance
cur.pop_into(tmp);       // copy 32 bytes, advance 32
```

### Zero-Copy View

```cpp
template<bounded_array   U> std::span<const std::remove_all_extents_t<U>> as_view() noexcept;
template<binary_readable T> std::span<const T> as_view(usize count) noexcept;
```

```cpp
auto s1 = cur.as_view<u32[8]>();  // span of 8 u32s
auto s2 = cur.as_view<u8>(64);    // span of 64 bytes
```

### String View

```cpp
std::string_view read_strvw() noexcept;                        // to end
std::string_view read_strvw(usize max) noexcept;               // up to max bytes
```

Reads a null-terminated (or bounded) string from the current position.
Advances past the null terminator if found.

```cpp
auto name = cur.read_strvw();          // read null-terminated string
auto name = cur.read_strvw(256);       // read up to 256 bytes
```

### Span Access

```cpp
std::span<ByteType>       bytes() noexcept;
std::span<const ByteType> bytes() const noexcept;
```

Returns a span of the remaining buffer from cursor to end.

```cpp
auto remaining = cur.bytes();          // span from cursor to end
for (auto& b : remaining) { /* ... */ }
```

### Pointer at Cursor

```cpp
template<typename T = ByteType> constexpr ptr<T> as_p() const noexcept;
```

```cpp
auto p = cur.as_p<u32>();  // ptr<u32> at current cursor position
```

---

## `map_file`

Memory-maps a file using OS primitives (`mmap` on Linux, `CreateFileMapping`
on Windows). Inherits privately from `memcur<std::byte>`.

### Factory (static open)

```cpp
static auto open(const std::filesystem::path& path, map_flag flags = {}) noexcept
    -> std::expected<map_file, std::errc>;

static auto open(const std::filesystem::path& path, off_s offset, usize size, map_flag flags = {}) noexcept
    -> std::expected<map_file, std::errc>;
```

```cpp
// Map entire file read-only
auto mapping = map_file::open("/path/to/file.bin");
if (!mapping) { /* handle error */ }

// Map region 0x1000..0x2000 with write
auto mapping2 = map_file::open("/path/to/file.bin",
    off_s{0x1000}, 0x1000,
    map_flag::write);
```

### Move-Only

```cpp
map_file(map_file&&) noexcept;
map_file& operator=(map_file&&) noexcept;
map_file(const map_file&) = delete;
```

### Re-exported from `memcur`

| Category | Members                                             |
|----------|-----------------------------------------------------|
| State    | `operator bool`, `size()`, `base()`                 |
| Cursor   | `seek()`, `advance()`, `tell()`, `remaining()`      |
| Read     | `pop()`, `as_view()`, `read_into()`, `read_strvw()` |
| Write    | `push()`, `pop_into()`                              |
| Access   | `bytes()`, `as_p()`                                 |

```cpp
auto mapping = map_file::open("file.bin", map_flag::write);

// Parse sequentially
auto magic = mapping.pop<u32>();
auto type  = mapping.pop<u16>();

// Zero-copy view
auto header = mapping.as_view<u8[64]>();

// Discard unwanted fields:
null << mapping.pop<u32>();  // suppress nodiscard
null << mapping.pop<u16>();
```

### Map-Specific

```cpp
map_flag flags()    const noexcept;
bool     is_alive() const noexcept;
void     swap(map_file&) noexcept;
auto flush() noexcept -> std::expected<void, std::errc>;
```

```cpp
if (mapping.is_alive()) {
    auto fl = mapping.flags();   // map_flag::write
    mapping.flush();             // msync / FlushViewOfFile
}
```

---

## Deduction Guides (memcur)

```cpp
memcur(B*, usize) -> memcur<B>;                                    // when B satisfies buffer_type
memcur(T*, usize) -> memcur<std::conditional_t<std::is_const_v<T>, const std::byte, std::byte>>;  // otherwise
```

Allows constructing `memcur` from a raw pointer without specifying the template argument explicitly:

```cpp
auto raw = map_file::open("file.bin");
auto ptr = raw->as_p<u32>();   // ptr<u32>

memcur cur{ptr, 1024};         // deduces memcur<std::byte> via second guide
memcur const_cur{ptr, 1024};   // memcur<const std::byte>
```

The first guide applies when the type already models `buffer_type` (e.g. `std::byte`, `char`). The second guide wraps any non-buffer pointer into `std::byte` or `const std::byte`.

---

## `fs::read` / `fs::write` (map_file overloads)

Positional access into a `map_file` without moving its cursor.

```cpp
template<binary_readable Type>
std::expected<Type, std::errc> read(const map_file& m, off_s offset) noexcept;

template<binary_readable Type>
std::expected<void, std::errc> write(map_file& m, off_s offset, const Type& value) noexcept;
```

```cpp
auto m = map_file::open("file.bin");
auto magic = fs::read<u32>(*m, off_s{0});
```

---

## `fs::read` / `fs::write` (span overloads)

Positional access into a `std::span<std::byte>`.

```cpp
template<binary_readable Type>
std::expected<Type, std::errc> read(std::span<const std::byte> buf, off_s offset) noexcept;

template<binary_readable Type>
std::expected<void, std::errc> write(std::span<std::byte> buf, off_s offset, const Type& value) noexcept;
```

```cpp
std::vector<std::byte> buf(1024);
auto v = fs::read<u32>(std::span<const std::byte>{buf}, off_s{0});
```

