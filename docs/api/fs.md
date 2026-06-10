# fs.hpp

## `dirty_vector`

A `std::vector` with an allocator that does NOT zero-initialize elements.
Useful for buffers that will be overwritten entirely.

```cpp
template<binary_readable Type = u8>
using dirty_vector = std::vector<Type, details::vec_init_allocator<Type>>;
```

```cpp
auto vec = stx::dirty_vector<stx::u8>(1024);  // uninitialized memory
read(fd, vec.data(), vec.size());              // overwrite, no wasted zeroing
```

---

## `fs::origin`

Seek-origin enum for `memcur::seek` / `memcur::skip`.

| Enumerator | Behavior |
|------------|----------|
| `origin::begin` | Seek from start |
| `origin::current` | Seek from current position |
| `origin::end` | Seek from end |

```cpp
using stx::fs::origin;
```

---

## `fs::readfs` / `fs::writefs` (stream I/O)

Positional read/write from/to `std::istream`/`std::ostream`.

### Single-value read

```cpp
template<binary_readable Type>
std::expected<Type, std::errc> readfs(std::istream& file, off_s offset = {}, origin dir = origin::begin) noexcept;
```

### Buffer read (span, count, or fixed array)

```cpp
template<binary_readable Type>
std::expected<void, std::errc> readfs(std::istream&, std::span<Type>, off_s, origin = begin) noexcept;

template<binary_readable Type>
std::expected<dirty_vector<Type>, std::errc> readfs(std::istream&, off_s, usize count, origin = begin) noexcept;

template<binary_readable Type, usize Size>
std::expected<std::array<Type, Size>, std::errc> readfs(std::istream&, off_s = {}, origin = begin) noexcept;
```

### Single-value write

```cpp
template<binary_readable Type>
std::expected<void, std::errc> writefs(std::ostream&, off_s, const Type& value) noexcept;
```

### Buffer write

```cpp
template<binary_readable Type>
std::expected<void, std::errc> writefs(std::ostream&, off_s, std::span<const Type>) noexcept;
```

### Navigation helpers

```cpp
void setposfs(std::istream&, off_s, origin = begin) noexcept;
void skipfs(std::istream&, off_s) noexcept;
void setposos(std::ostream&, off_s, origin = begin) noexcept;
void skipos(std::ostream&, off_s) noexcept;
```

```cpp
std::ifstream file{"data.bin", std::ios::binary};

// Single value at offset
auto magic = stx::fs::readfs<stx::u32>(file, stx::off_s{0});
if (magic) {
    // use magic.value()
}

// Read into a buffer
std::vector<stx::u8> buf(1024);
auto ok = stx::fs::readfs(file, std::span{buf}, stx::off_s{0x100});

// Read fixed array
auto arr = stx::fs::readfs<stx::u8, 16>(file, stx::off_s{0x200});

// Write
stx::fs::writefs(file, stx::off_s{0}, 0xDEADBEEF);
```

---

## `map_flag`

Flags controlling `map_file::open` behavior.

| Flag | Effect |
|------|--------|
| `map_flag::none` | Read-only, private, no populate |
| `map_flag::write` | Read-write mapping |
| `map_flag::exec` | Executable mapping |
| `map_flag::shared` | Share with other processes (MAP_SHARED) |
| `map_flag::priv` | Copy-on-write (MAP_PRIVATE) |
| `map_flag::populate` | Pre-populate page tables (MAP_POPULATE, Linux) |

```cpp
stx::map_flag flags = stx::map_flag::write | stx::map_flag::shared;
```

---

## `memcur<ByteType>`

A non-owning cursor over a contiguous range of byte-like elements. Stores
a `ptr<ByteType>` for the base and a `ptr<ByteType>` for the current
position. `ByteType` must satisfy `buffer_type` (`char`, `std::byte`,
`u8`, `i8`).

### State

| Method | Returns | Description |
|--------|---------|-------------|
| `base()` | `uptr` | Start address of the buffer |
| `size()` | `usize` | Total size in elements |
| `tell()` | `off_s` | Current byte offset from base |
| `remaining()` | `off_s` | Remaining bytes from cursor to end |
| `operator bool()` | `bool` | Non-null check |

```cpp
stx::memcur cur{buf, 256};
cur.skip(stx::off_s{32});
auto pos = cur.tell();        // off_s{32}
auto rem = cur.remaining();   // off_s{224}
```

### Cursor Movement

```cpp
void seek(off_s, origin dir = origin::begin) noexcept;
void skip(off_s) noexcept;
```

```cpp
cur.seek(stx::off_s{0});              // rewind to start
cur.seek(stx::off_s{-16}, stx::origin::end);  // 16 bytes before end
cur.skip(stx::off_s{8});              // advance 8 bytes
```

### Pop (read + advance)

```cpp
template<binary_readable T> T pop() noexcept;
template<bounded_array U> details::bounded_array_t<U> pop() noexcept;
```

```cpp
auto a = cur.pop<stx::u32>();        // read u32, advance 4
auto b = cur.pop<stx::u16>();        // read u16, advance 2
auto arr = cur.pop<stx::u8[16]>();   // read 16 bytes, advance 16
```

### Push (write + advance)

```cpp
template<binary_readable T> auto& push(const T& value) noexcept;
template<contiguous_buffer R> auto& push(R&& buf) noexcept;
```

Returns `*this` for chaining.

```cpp
cur.push(stx::u32{0xDEAD}).push(stx::u16{0xBEEF});
cur.push(std::vector{stx::u8{1, 2, 3, 4}});
```

### Read Into / Pop Into

```cpp
template<writable_buffer R> void   read_into(R&& buf) noexcept;
template<writable_buffer R> auto&  pop_into(R&& buf) noexcept;
```

```cpp
std::array<stx::u8, 32> tmp;
cur.read_into(tmp);          // copy 32 bytes, no advance
cur.pop_into(tmp);           // copy 32 bytes, advance 32
```

### Zero-Copy View

```cpp
template<bounded_array U> std::span<const std::remove_all_extents_t<U>> as_view() noexcept;
template<binary_readable T> std::span<const T> as_view(usize count) noexcept;
```

```cpp
auto s1 = cur.as_view<stx::u32[8]>();  // span of 8 u32s
auto s2 = cur.as_view<stx::u8>(64);    // span of 64 bytes
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
auto p = cur.as_p<stx::u32>();  // ptr<u32> at current cursor position
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
auto mapping = stx::map_file::open("/path/to/file.bin");
if (!mapping) { /* handle error */ }

// Map region 0x1000..0x2000 with write
auto mapping2 = stx::map_file::open("/path/to/file.bin",
    stx::off_s{0x1000}, 0x1000,
    stx::map_flag::write);
```

### Move-Only

```cpp
map_file(map_file&&) noexcept;
map_file& operator=(map_file&&) noexcept;
map_file(const map_file&) = delete;
```

### Re-exported from `memcur`

| Category | Members |
|----------|---------|
| State | `operator bool`, `size()`, `base()` |
| Cursor | `seek()`, `skip()`, `tell()`, `remaining()` |
| Read | `pop()`, `as_view()`, `read_into()`, `read_strvw()` |
| Write | `push()`, `pop_into()` |
| Access | `bytes()`, `as_p()` |

```cpp
auto mapping = stx::map_file::open("file.bin", stx::map_flag::write);

// Parse sequentially
auto magic = mapping.pop<stx::u32>();
auto type  = mapping.pop<stx::u16>();

// Zero-copy view
auto header = mapping.as_view<stx::u8[64]>();

// Discard unwanted fields:
stx::null << mapping.pop<stx::u32>();  // suppress nodiscard
stx::null << mapping.pop<stx::u16>();
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
    auto fl = mapping.flags();          // map_flag::write
    mapping.flush();                    // msync / FlushViewOfFile
}
```

---

## `fs::readfs` / `fs::writefs` (map_file overloads)

Positional access into a `map_file` without moving its cursor.

```cpp
template<binary_readable Type>
std::expected<Type, std::errc> readfs(const map_file& m, off_s offset) noexcept;

template<binary_readable Type>
std::expected<void, std::errc> writefs(map_file& m, off_s offset, const Type& value) noexcept;
```

```cpp
auto m = stx::map_file::open("file.bin");
auto magic = stx::fs::readfs<stx::u32>(*m, stx::off_s{0});
```

---

## `fs::readfs` / `fs::writefs` (span overloads)

Positional access into a `std::span<std::byte>`.

```cpp
template<binary_readable Type>
std::expected<Type, std::errc> readfs(std::span<const std::byte> buf, off_s offset) noexcept;

template<binary_readable Type>
std::expected<void, std::errc> writefs(std::span<std::byte> buf, off_s offset, const Type& value) noexcept;
```

```cpp
std::vector<std::byte> buf(1024);
auto v = stx::fs::readfs<stx::u32>(std::span<const std::byte>{buf}, stx::off_s{0});
```
