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
Type readfs(
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

header h = stx::readfs<header>(file, stx::off_s{0});
```

---

# Reading Into Span

## `readfs<Type>(istream&, std::span<Type>)`

```cpp
template<binary_readable Type>
void readfs(
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
dirty_vector<Type> readfs(
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

auto words = stx::readfs<stx::u16>(file, stx::off_s{128}, 64);
```

---

# Reading Fixed-Size Array

## `readfs<Type, Size>(istream&, off_s, origin)`

```cpp
template<binary_readable Type, usize Size>
std::array<Type, Size> readfs(
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
```

---

# Skipping Bytes

## `skipfs`

```cpp
template<binary_readable Type = std::byte>
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

# Stream State

## `last_read_ok`

```cpp
bool last_read_ok(const std::istream& file) noexcept;
```

Returns:

```cpp
file.good() and not file.eof()
```

### Example

```cpp
auto value = stx::readfs<stx::u32>(file);

if (!stx::last_read_ok(file)) {
    // handle error
}
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
template<binary_readable Type = u8>
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
| `size()` | `usize` | Mapped region size |
| `base()` | `uptr` | Base address |
| `flags()` | `map_flag` | Access flags |

### Sequential I/O

| Member | Description |
|--------|-------------|
| `seek(off_s, origin)` | Set sequential position |
| `tell()` | Current sequential position |
| `remaining()` | Remaining bytes |
| `read<T>()` | Sequential read (advances position) |
| `write<T>(const T&)` | Sequential write (advances position) |

### Random-Access I/O

| Member | Description |
|--------|-------------|
| `read<T>(off_s)` | Read at byte offset |
| `write<T>(off_s, const T&)` | Write at byte offset |

### View

| Member | Returns | Description |
|--------|---------|-------------|
| `bytes()` | `span<const byte>` / `span<byte>` | Full region as span |
| `as_p<T>()` | `ptr<T>` | Typed pointer to base |

### Example

```cpp
auto mapped = stx::map_file::open("data.bin", stx::map_flag::write);
if (!mapped) return;

auto& m = *mapped;

// Random access
stx::u32 val = m.read<stx::u32>(stx::off_s{0x100});
m.write(stx::off_s{0x104}, 0xDEADBEEF);

// Sequential
m.seek(stx::off_s{0x100});
stx::u32 a = m.read<stx::u32>();
stx::u32 b = m.read<stx::u32>();

// As span
auto region = m.bytes();

// As typed pointer
auto ptr = m.as_p<stx::u32>();
```

### `readfs` / `writefs` Overloads for `map_file`

```cpp
template<binary_readable Type>
std::expected<Type, std::errc> readfs(const map_file& m, const off_s offset);

template<binary_readable Type>
std::expected<void, std::errc> writefs(map_file& m, const off_s offset, const Type& value);
```

---

# Safety Characteristics

| Aspect                     | Guarantee |
|----------------------------|----------|
| Type safety                | Enforced via `binary_readable` |
| Offset correctness         | Enforced via `off_s` |
| No implicit domain mixing  | Yes |
| Bounds checking            | No |
| Endianness handling        | Not provided |
| Stream state validation    | Explicit via `last_read_ok` |

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
