# fs.hpp

## Overview

`fs.hpp` provides binary-oriented utilities over `std::istream`, designed for structured file parsing under C++23. It builds on:

- `binary_readable`
- `offset_t`
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
    const offset_t offset,
    const origin dir = origin::begin
);
```

### Description

Sets the read position of a stream using strong-typed offset and custom origin enum.

Internally converts:

- `offset_t` → `std::streamoff`
- `origin` → `std::ios_base::seekdir`

### Example

```cpp
std::ifstream file("data.bin", std::ios::binary);

stx::setposfs(file, stx::offset_t{128});
```

Move relative to current position:

```cpp
stx::setposfs(file, stx::offset_t{32}, stx::origin::current);
```

---

# Reading Single Object

## `readfs<Type>(istream&, offset_t, origin)`

```cpp
template<binary_readable Type>
Type readfs(
    std::istream& file,
    const offset_t offset = offset_t{},
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

header h = stx::readfs<header>(file, stx::offset_t{0});
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

## `readfs<Type>(istream&, offset_t, usize, origin)`

```cpp
template<binary_readable Type = u8>
dirty_vector<Type> readfs(
    std::istream& file,
    const offset_t offset,
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
auto bytes = stx::readfs<>(file, stx::offset_t{0}, 256);

auto words = stx::readfs<stx::u16>(file, stx::offset_t{128}, 64);
```

---

# Reading Fixed-Size Array

## `readfs<Type, Size>(istream&, offset_t, origin)`

```cpp
template<binary_readable Type, usize Size>
std::array<Type, Size> readfs(
    std::istream& file,
    const offset_t offset,
    const origin dir = origin::begin
);
```

Constraint:

- `Size > 0`

### Example

```cpp
auto block = stx::readfs<stx::u8, 64>(file, stx::offset_t{0});
```

---

# Skipping Bytes

## `skipfs`

```cpp
template<binary_readable Type = std::byte>
void skipfs(
    std::istream& file,
    const offset_t offset
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
stx::skipfs(file, stx::offset_t{32});
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

# Safety Characteristics

| Aspect                     | Guarantee |
|----------------------------|----------|
| Type safety                | Enforced via `binary_readable` |
| Offset correctness         | Enforced via `offset_t` |
| No implicit domain mixing  | Yes |
| Bounds checking            | No |
| Endianness handling        | Not provided |
| Stream state validation    | Explicit via `last_read_ok` |

---

# Intended Usage

- Binary file parsing
- Structured header extraction
- Section-based file inspection
- Memory-mapped stream adapters
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
