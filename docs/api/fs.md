# fs.hpp

## Overview

`fs.hpp` provides binary-oriented utilities over `std::istream`, designed for structured file parsing under C++23. It builds on:

- `binary_readable`
- `off_s`
- `origin`
- Strong typing from `core.hpp`

The API focuses on:

- Strongly-typed file offsets
- Safe binary reads
- Minimal abstraction overhead
- Explicit positioning semantics

---

# Internal Utility

## `details::vec_init_allocator`

Custom allocator that default-constructs elements when possible but avoids unnecessary value-initialization overhead.

### Purpose

`std::vector<T>(n)` value-initializes elements.
For POD-like binary buffers, this is unnecessary.

`vec_init_allocator`:

- Constructs default-constructible types using placement new.
- Delegates parameterized construction to underlying allocator.
- Avoids forced zero-initialization where not required.

### Key Behavior

```cpp
template<typename U>
void construct(U* ptr)
```

If `U` is default-constructible, it performs:

```cpp
::new(static_cast<void*>(ptr)) U;
```

This enables high-performance uninitialized buffer allocation suitable for binary reads.

---

# Type Alias

## `dirty_vector<Type>`

```cpp
template<binary_readable Type = u8>
using dirty_vector = std::vector<Type, details::vec_init_allocator<Type>>;
```

### Characteristics

| Property                  | Description |
|---------------------------|-------------|
| Constraint                | `Type` must satisfy `binary_readable` |
| Allocation strategy       | Uses `vec_init_allocator` |
| Intended usage            | Raw binary buffers |

Example:

```cpp
stx::dirty_vector<stx::u32> buffer(100);
```

---

# Positioning

## `setposfs`

```cpp
void setposfs(
    std::istream& file,
    const off_s offset,
    const origin dir = origin::begin
);
```

### Description

Sets the read position of a stream using strong-typed offset and custom origin enum.

Internally converts:

- `off_s` → `std::streamoff`
- `origin` → `std::ios_base::seekdir`

### Example

```cpp
std::ifstream file("data.bin", std::ios::binary);

stx::setposfs(file, stx::off_s{128});
```

Move relative to current position:

```cpp
stx::setposfs(file, stx::off_s{32}, stx::origin::current);
```

---

# Reading Single Object

## `readfs<Type>(istream&, off_s, origin)`

```cpp
template<binary_readable Type>
std::expected<Type, std::errc> readfs(
    std::istream& file,
    const off_s offset = off_s{},
    const origin dir = origin::begin
);
```

### Behavior

1. Seeks to specified position.
2. Reads `sizeof(Type)` bytes.
3. Returns value by copy.

### Example

```cpp
struct header {
    stx::u32 magic;
    stx::u16 version;
    stx::u16 flags;
};

static_assert(stx::binary_readable<header>);

auto h = stx::readfs<header>(file, stx::off_s{0});
if (!h) return;
```

---

# Reading Into Span

## `readfs<Type>(istream&, std::span<Type>)`

```cpp
template<binary_readable Type>
std::expected<void, std::errc> readfs(
    std::istream& file,
    std::span<Type> out_buffer
);
```

### Behavior

Reads directly into existing buffer.

### Example

```cpp
std::array<stx::u32, 16> arr;

stx::readfs(file, std::span{arr});
```

---

# Reading Into `dirty_vector`

## `readfs<Type>(istream&, off_s, usize, origin)`

```cpp
template<binary_readable Type = u8>
std::expected<dirty_vector<Type>, std::errc> readfs(
    std::istream& file,
    const off_s offset,
    const usize count,
    const origin dir = origin::begin
);
```

### Behavior

1. Allocates `dirty_vector<Type>` with `count` elements.
2. Seeks to position.
3. Reads `count * sizeof(Type)` bytes.
4. Returns vector.

### Example

```cpp
auto bytes = stx::readfs<>(file, stx::off_s{0}, 256);
if (!bytes) return;

auto words = stx::readfs<stx::u16>(file, stx::off_s{128}, 64);
if (!words) return;
```

---

# Reading Fixed-Size Array

## `readfs<Type, Size>(istream&, off_s, origin)`

```cpp
template<binary_readable Type, usize Size>
std::expected<std::array<Type, Size>, std::errc> readfs(
    std::istream& file,
    const off_s offset,
    const origin dir = origin::begin
);
```

Constraint:

- `Size > 0`

### Example

```cpp
auto block = stx::readfs<stx::u8, 64>(file, stx::off_s{0});
if (!block) return;
```

---

# Skipping Bytes

## `skipfs`

```cpp
void skipfs(
    std::istream& file,
    const off_s offset
);
```

### Behavior

Moves the stream position relative to current position.

Equivalent to:

```cpp
seekg(offset, std::ios::cur)
```

### Example

```cpp
stx::skipfs(file, stx::off_s{32});
```

---

---

# Write Positioning

## `setposos`

```cpp
void setposos(
    std::ostream& file,
    const off_s offset,
    const origin dir = origin::begin
);
```

### Description

Sets the write position of an output stream using strong-typed offset and origin.

Internally converts:

- `off_s` → `std::streamoff`
- `origin` → `std::ios_base::seekdir`

### Example

```cpp
std::ofstream file("out.bin", std::ios::binary);

stx::setposos(file, stx::off_s{128});
```

---

# Writing Single Object

## `writefs<Type>(ostream&, off_s, const Type&)`

```cpp
template<binary_readable Type>
std::expected<void, std::errc> writefs(
    std::ostream& file,
    const off_s offset,
    const Type& value
);
```

### Behavior

1. Seeks to specified position.
2. Writes `sizeof(Type)` bytes from `value`.

### Example

```cpp
stx::u32 magic = 0xDEADBEEF;
auto result = stx::writefs(file, stx::off_s{0}, magic);
```

---

# Writing Span

## `writefs<Type>(ostream&, off_s, span<const Type>)`

```cpp
template<binary_readable Type>
std::expected<void, std::errc> writefs(
    std::ostream& file,
    const off_s offset,
    std::span<const Type> buffer
);
```

### Behavior

1. Seeks to specified position.
2. Writes `buffer.size() * sizeof(Type)` bytes.

### Example

```cpp
std::array<stx::u32, 4> data{1, 2, 3, 4};
auto result = stx::writefs(file, stx::off_s{0}, std::span{data});
```

---

# Skipping Bytes (Write)

## `skipos`

```cpp
void skipos(
    std::ostream& file,
    const off_s offset
);
```

### Behavior

Moves the write position relative to current position.

Equivalent to:

```cpp
seekp(offset, std::ios::cur)
```

### Example

```cpp
stx::skipos(file, stx::off_s{32});
```

---

# Memory-Mapped Files

## `map_flag`

```cpp
enum class map_flag : u8 {
    none     = 0,
    write    = 1 << 0,
    exec     = 1 << 1,
    shared   = 1 << 2,
    priv     = 1 << 3,
    populate = 1 << 4,
};
```

| Flag | Effect |
|------|--------|
| `none` | Read-only, private |
| `write` | Read-write mapping |
| `exec` | Executable mapping |
| `shared` | `MAP_SHARED` (changes visible to other processes) |
| `priv` | `MAP_PRIVATE` (copy-on-write) |
| `populate` | `MAP_POPULATE` (pre-fault pages, Linux) |

Flags can be combined with `operator|`:

```cpp
stx::map_flag flags = stx::map_flag::write | stx::map_flag::shared;
```

Check flags with `operator&`:

```cpp
if (m.flags() & stx::map_flag::write) { ... }
```

---

## `map_file`

```cpp
class map_file
```

RAII memory-mapped file wrapper. Uses `mmap` on POSIX, `CreateFileMapping`/`MapViewOfFile` on Windows.

### Factory

```cpp
static auto open(const std::filesystem::path& path, map_flag flags = {})
    -> std::expected<map_file, std::errc>;

static auto open(const std::filesystem::path& path, off_s offset, usize size, map_flag flags = {})
    -> std::expected<map_file, std::errc>;
```

### State

| Member | Returns | Description |
|--------|---------|-------------|
| `operator bool` | `bool` | Non-null check |
| `is_alive()` | `bool` | `data_ != nullptr` (same as `operator bool`) |
| `size()` | `usize` | Mapped region size |
| `base()` | `uptr` | Base address |
| `flags()` | `map_flag` | Access flags |

### Sequential I/O

| Member | Description |
|--------|-------------|
| `seek(off_s, origin)` | Set sequential position |
| `skip(off_s)` | Advance relative to current (`seek(off, origin::current)`) |
| `tell()` | Current sequential position |
| `remaining()` | Remaining bytes |

### Pop (read + advance cursor)

| Member | Returns | Description |
|--------|---------|-------------|
| `pop<T>()` | `T` | Single value (advances cursor) |
| `pop<T, N, Rest...>()` | nested `array` | Multi-dimensional read, advances by total size |

### Push (write + advance cursor)

| Member | Description |
|--------|-------------|
| `push<T>(val)` | Write single value, advances cursor |
| `push<T>(span)` | Write span elements, advances cursor |
| `push(sv)` | Write `string_view` bytes, advances cursor |

### Zero-Copy Views

| Member | Returns | Description |
|--------|---------|-------------|
| `bytes()` | `span<const byte>` / `span<byte>` | Full region as span |
| `as_p<T>()` | `ptr<T>` | Typed pointer to base |
| `read_view<T>(count)` | `span<const T>` | View of `count` elements at cursor (zero-copy, advances cursor) |
| `read_strvw()` | `string_view` | Scans for `\0` until end of buffer (zero-copy, advances cursor) |
| `read_strvw(max)` | `string_view` | Scans for `\0` bounded by `max` bytes (zero-copy, advances cursor) |

> **Semantics:** scans forward from cursor for a null terminator (`\0`) within the available range. If found, cursor advances past the null and returns the string before it. If not found, cursor advances by the scanned amount and returns everything scanned. Valid while the `map_file` is alive — views become dangling on move/destroy.

### Example

```cpp
auto mapped = stx::map_file::open("data.bin", stx::map_flag::write);
if (!mapped) return;

auto& m = *mapped;

// Random access via ptr
stx::u32 val = m.as_p<stx::u32>()[stx::off_s{0x100}].read<stx::u32>();
m.as_p<stx::u32>()[stx::off_s{0x104}].write(stx::u32{0xDEADBEEF});

// Sequential
m.seek(stx::off_s{0x100});
stx::u32 a = m.pop<stx::u32>();
stx::u32 b = m.pop<stx::u32>();

// Skip bytes
m.skip(stx::off_s{8});

// Zero-copy view of structs
struct Entry { stx::u32 id; stx::u64 off; };
auto entries = m.read_view<Entry>(4);

// Zero-copy string (bounded, null-terminated)
auto name = m.read_strvw(32);

// Copy into vector
auto buf = m.read<stx::u8>(256);

// As typed pointer
auto p = m.as_p<stx::u32>();
```

### `readfs` / `writefs` Overloads for `map_file`

```cpp
template<binary_readable Type>
std::expected<Type, std::errc> readfs(const map_file& m, const off_s offset);

template<binary_readable Type>
std::expected<void, std::errc> writefs(map_file& m, const off_s offset, const Type& value);
```

---

## `reader_view` (Bounded)

Cursor-based binary reader over an existing buffer with known size. Zero-copy — views point directly into the buffer.

```cpp
class reader_view {
    reader_view() noexcept;
    reader_view(std::span<std::byte> buf) noexcept;
    reader_view(void* data, usize size) noexcept;
    reader_view(const void* data, usize size) noexcept;
};
```

### State

| Member | Returns | Description |
|--------|---------|-------------|
| `operator bool` | `bool` | Buffer non-empty |
| `size()` | `usize` | Buffer size |
| `bytes()` | `span<const byte>` | Full buffer as span |
| `as_p<T>()` | `ptr<T>` | Typed pointer to buffer base |

### Cursor

| Member | Description |
|--------|-------------|
| `seek(off_s, origin)` | Set position (clamped to buffer) |
| `skip(off_s)` | Advance relative (`seek(off, current)`) |
| `tell()` | Current position |
| `remaining()` | Remaining bytes |

### Pop (read + advance cursor)

| Member | Returns | Description |
|--------|---------|-------------|
| `pop<T>()` | `T` | Single value (advances cursor) |
| `pop<T, N, Rest...>()` | nested `array` | Multi-dimensional read, advances by total size |

### Push (write + advance cursor)

| Member | Description |
|--------|-------------|
| `push<T>(val)` | Write single value (advances cursor) |
| `push<T>(span)` | Write span of values (advances cursor) |
| `push(sv)` | Write `string_view` bytes (advances cursor) |

### Read (zero-copy views, advances cursor)

| Member | Returns | Description |
|--------|---------|-------------|
| `read_view<T>(count)` | `span<const T>` | Zero-copy view of `count` elements (advances cursor) |
| `read_strvw()` | `string_view` | Scan for `\0` until end of buffer (zero-copy) |
| `read_strvw(max)` | `string_view` | Scan for `\0` bounded by `max` bytes (zero-copy) |

> **Note:** The zero-copy `read_view` and `read_strvw` methods advance the cursor by the viewed region size.

### Example (reader_view)

```cpp
int data[] = {1, 2, 3, 4};
stx::reader_view r(data, sizeof(data));

// Sequential pop
auto x = r.pop<int>();     // reads int, advances by 4

// Random access via as_p
auto val = r.as_p<int>()[off_s{8}].read<int>();

// Zero-copy view
auto span = r.read_view<int>(2);

// String view from raw data
auto sv = r.read_strvw();
```

> **⚠️ Lifetime:** `read_view` and `read_strvw` return non-owning views valid only while the source buffer outlives the view.

### Example

```cpp
std::vector<std::byte> buf = /* ... */;
stx::reader_view r{buf};

// Single reads
auto magic = r.read<stx::le<stx::u32>>();
auto count = r.read<stx::u16>();

// Zero-copy view
auto entries = r.read_view<Entry>(count);

// Copy into vector
auto names = r.read<char>(32);

// String
auto name = r.read_strvw();

// Write
r.write<stx::u32>(0xDEADBEEF);
```

---

## C/C++ Comparison: `map_file` vs Raw `mmap`

```cpp
// stx:  RAII, bounded cursor, zero-copy views
auto mapped = stx::map_file::open("data.bin");
if (!mapped) return;
auto& m = *mapped;

stx::u32 magic = m.as_p<stx::u32>()[stx::off_s{0}].read<stx::u32>();
m.seek(stx::off_s{8});
auto entries = m.read_view<Entry>(count);
auto name = m.read_strvw();
// closes automatically — no cleanup
```

```c
// C:  manual open, mmap, pointer math, munmap
int fd = open("data.bin", O_RDONLY);
struct stat st;
fstat(fd, &st);
void* data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
close(fd);

u32 magic = *(u32*)((char*)data + 0);
// must track offset manually
size_t off = 8;
Entry* entries = (Entry*)((char*)data + off);
off += count * sizeof(Entry);
const char* name = (const char*)data + off;
size_t len = strnlen(name, st.st_size - off);
// munmap at the end — easy to forget or exception-unsafe
munmap(data, st.st_size);
```

`map_file` is RAII (no leak), provides a cursor (`seek`/`tell`/`skip`), and returns type-safe views.

---

## C/C++ Comparison: `readfs` vs Raw `fread`

```cpp
// stx:  one call, bounds-checked
auto h = stx::readfs<header>(file, stx::off_s{0});
if (!h) return;
```

```c
// C:  manual seek, read, error check
struct header h;
fseek(file, 0, SEEK_SET);
if (fread(&h, sizeof(h), 1, file) != 1)
{
    fclose(file);
    return;
}
```

```cpp
// C++ raw:  seekg + read + fail check
std::ifstream file("data.bin", std::ios::binary);
header h;
file.seekg(0);
if (!file.read(reinterpret_cast<char*>(&h), sizeof(h)))
    return;
```

`readfs` returns `std::expected` — no streams to fail/clear, no casts, no manual size calculation.

---

## C/C++ Comparison: `readfs` over `span` vs Manual Bounds

```cpp
// stx:  bounds-checked, typed, zero-overhead
auto val = stx::readfs<u32>(buf, stx::off_s{0x100});
if (!val) return;
```

```c
// C:  caller manages everything
if (0x100 + sizeof(u32) > buf_size) return;
u32 val;
memcpy(&val, (char*)buf + 0x100, sizeof(val));
```

```cpp
// C++ raw:  same cost, more lines
if (off_s{0x100}.get() + sizeof(u32) > buf.size()) return;
u32 val;
std::memcpy(&val, buf.data() + 0x100, sizeof(u32));
```

`readfs<Type>(span, off)` performs the same bounds check and `memcpy` — but the caller writes one line instead of four.

---

# Safety Characteristics

| Aspect                     | Guarantee |
|----------------------------|----------|
| Type safety                | Enforced via `binary_readable` |
| Offset correctness         | Enforced via `off_s` |
| No implicit domain mixing  | Yes |
| Bounds checking            | No |
| Endianness handling        | Not provided |
| Stream state validation    | Explicit via `!file.fail()` |

---

# Intended Usage

- Binary file parsing
- Structured header extraction
- Section-based file inspection
- Memory-mapped file analysis
- Low-level file analysis tooling

---

# Design Notes

- No dynamic polymorphism.
- No exception handling layer.
- No implicit endian transformation.
- No hidden buffering.
- Minimal runtime overhead.

The API assumes the caller:

- Opens the stream in binary mode.
- Ensures sufficient file size.
- Validates stream state where required.

The design remains low-level and explicit, suitable for systems and binary tooling contexts.
