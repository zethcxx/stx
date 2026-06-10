# fs.hpp

## `origin`

Seek-origin enum controlling `readfs` / `writefs` positioning.

| Enumerator | Value | Behavior |
|------------|-------|----------|
| `origin::begin` | `SEEK_SET` | Seek from start of file |
| `origin::current` | `SEEK_CUR` | Seek from current position |
| `origin::end` | `SEEK_END` | Seek from end of file |

```cpp
enum class origin : stx::u8
{
    begin   = SEEK_SET,
    current = SEEK_CUR,
    end     = SEEK_END
};
```

---

## `memcur<ByteType>`

A non-owning cursor over a contiguous range of byte-like elements.
`ByteType` must satisfy `buffer_type`. Tracks position in both elements
and bytes, with stride-awareness when reading/writing structured data.

```
  ByteType*  0  1  2  3  4  5  ...  len-1
            [  |  |  |  |  |  |  ...  |  ]
             ^
          offset() = 0
          position() = 0
```

### Type Parameters

| Parameter | Constraint | Default | Description |
|-----------|------------|---------|-------------|
| `ByteType` | `buffer_type` | (deduced) | Element type of the underlying buffer |

### Construction

```cpp
memcur() noexcept;
memcur(ByteType* p, usize size_in_elements) noexcept;
template<address_like Ptr> memcur(Ptr p, usize len) noexcept;
memcur(const memcur&) noexcept;
memcur(memcur&&) noexcept;
```

```cpp
stx::u32 buffer[256];
stx::memcur cur{buffer, 256};     // CTAD -> memcur<u32>

stx::memcur<std::byte> empty;     // null cursor

// Address-like construction
stx::memcur cur2{some_ptr, 1024}; // memcur<std::byte> via CTAD
```

When constructed with an `address_like` pointer (e.g. `void*`, `uptr`),
the deduction guide selects `memcur<std::byte>`. When constructed with a
`buffer_type*` directly, it preserves that type.

### Position Queries

| Method | Returns | Unit |
|--------|---------|------|
| `offset()` | `usize` | Elements from start |
| `remain()` | `usize` | Elements remaining |
| `position()` | `usize` | Bytes from start |
| `bytes_remain()` | `usize` | Bytes remaining |

```cpp
stx::u32 arr[8];
stx::memcur cur{arr, 8};

auto off = cur.offset();       // 0
auto rem = cur.remain();       // 8
auto pos = cur.position();     // 0
auto brem = cur.bytes_remain(); // 32

cur.skip(3);                   // advance 3 elements
off = cur.offset();            // 3
pos = cur.position();          // 12 (= 3 * sizeof(u32))
rem = cur.remain();            // 5
```

### Cursor Movement

| Method | Argument | Unit | Behavior |
|--------|----------|------|----------|
| `seek(off_s)` | byte offset | Bytes | Absolute — jump to byte |
| `skip(off_s)` | byte delta | Bytes | Relative — advance/rewind in bytes |
| `skip(isize)` | element count | Elements | Relative — advance/rewind in elements |
| `reset()` | — | — | Go back to start (offset 0) |

```cpp
cur.seek(stx::off_s{16});   // jump to byte 16
cur.skip(stx::off_s{-8});   // back 8 bytes
cur.skip(4);                // forward 4 elements
cur.reset();                // back to start
```

### Reading

Single-value, bounded-array, and raw-buffer reads. The optional `off_s`
parameter reads at that byte offset without moving the cursor.

```cpp
template<binary_readable T> T        read(off_s off = {}) const;
template<bounded_array T>   auto     read(off_s off = {}) const;
void                               read(void* dest, usize count, off_s off = {}) const;
```

```cpp
// Read a single value at current position
auto v = cur.read<stx::u32>();

// Read at a specific byte offset (cursor unchanged)
auto v2 = cur.read<stx::u64>(stx::off_s{8});

// Read into a fixed-size array
auto arr = cur.read<stx::u8[16]>();   // std::array<u8, 16>

// Raw read into a buffer
stx::u8 tmp[32];
cur.read(tmp, sizeof(tmp));
```

### Writing

```cpp
template<binary_readable T> void    write(const T& value, off_s off = {});
template<bounded_array T>   void    write(const T& value, off_s off = {});
void                              write(const void* src, usize count, off_s off = {});
```

```cpp
cur.write<stx::u32>(0xDEADBEEF);
cur.write<stx::u64>(0x1234, stx::off_s{8});  // at byte offset 8

stx::u8 header[4] = {0x7F, 'E', 'L', 'F'};
cur.write(header, sizeof(header));
```

### Pop (Read + Advance)

Reads the value then advances the cursor by `sizeof(T)` bytes.

```cpp
template<binary_readable T> T        pop();
template<bounded_array T>   auto     pop();
```

```cpp
auto field1 = cur.pop<stx::u32>();   // read u32, advance 4 bytes
auto field2 = cur.pop<stx::u16>();   // read u16, advance 2 bytes
auto bytes  = cur.pop<stx::u8[8]>(); // read 8 bytes, advance 8
```

This makes sequential parsing natural:

```cpp
struct Header {
    stx::u32 magic;
    stx::u16 version;
    stx::u16 flags;
    stx::u32 size;
};

auto hdr = Header{
    .magic   = cur.pop<stx::u32>(),
    .version = cur.pop<stx::u16>(),
    .flags   = cur.pop<stx::u16>(),
    .size    = cur.pop<stx::u32>(),
};
```

### File I/O (readfs / writefs)

Reads from or writes to a file descriptor at a given offset. Combines
cursor position with file positioning.

| Method | Direction | Returns |
|--------|-----------|---------|
| `readfs(fd, off, whence)` | File → cursor | `usize` bytes read |
| `writefs(fd, off, whence)` | Cursor → file | `usize` bytes written |

```cpp
int fd = open("input.bin", O_RDONLY);

// Read 1024 bytes from fd at offset 0x200 into cursor
auto n = cur.readfs(fd, stx::off_s{0x200});

// Write cursor contents to fd 32 bytes before EOF
cur.writefs(fd, stx::off_s{-32}, stx::origin::end);
```

### Slicing

| Method | Returns | Description |
|--------|---------|-------------|
| `slice(off_s, count)` | `memcur` | Sub-range starting at byte `off`, `count` elements |
| `slice(off_s)` | `memcur` | Sub-range from byte `off` to end |
| `resize(sz)` | `void` | Truncate cursor to `sz` elements |
| `drop(n)` | `void` | Remove first `n` elements |

```cpp
auto sub = cur.slice(stx::off_s{8}, 4);   // 4 elements at byte 8
auto tail = cur.slice(stx::off_s{32});     // everything from byte 32
cur.drop(4);                               // discard first 4 elements
```

### Accessors

| Method | Returns |
|--------|---------|
| `data()` | `ByteType*` |
| `addr()` | `uptr` |
| `start_at(addr)` | New `memcur` shifted by `addr` bytes |

```cpp
auto d = cur.data();           // raw pointer to start
auto a = cur.addr();           // address as uptr
auto shifted = cur.start_at(stx::off_s{16});  // new cursor at byte 16
```

### Comparison

```cpp
explicit operator bool() const noexcept;  // true if non-null
bool operator==(const memcur&) const noexcept;
```

```cpp
if (cur) { /* non-null and valid */ }
if (cur == other) { /* same buffer and position */ }
```

---

## `map_file`

Memory-maps a file using OS primitives (`mmap` on Linux, `VirtualAlloc` on
Windows). Inherits privately from `memcur<std::byte>`, selectively
re-exporting the most useful members.

### Construction

```cpp
map_file(int fd, usize length, off_s offset = off_s{});
map_file(map_file&&) noexcept;
```

```cpp
int fd = open("binary.elf", O_RDONLY);
off_t sz = lseek(fd, 0, SEEK_END);
stx::map_file mapped{fd, static_cast<stx::usize>(sz)};
close(fd);
```

### Move Assignment / Destruction

```cpp
map_file& operator=(map_file&&) noexcept;
~map_file() noexcept;   // unmaps the file
```

### Re-exported Members

The following members of `memcur<std::byte>` are accessible through `map_file`:

| Category | Members |
|----------|---------|
| Position | `offset()`, `remain()` |
| Movement | `seek(off_s)`, `skip(off_s)` |
| Read | `read<T>()`, `read<T>(off_s)`, `read(dest, count)` |
| Read (array) | `read<bounded_array>()` |
| Write | `write<T>(val)`, `write<T>(val, off_s)`, `write(src, count)` |
| Write (array) | `write<bounded_array>()` |
| Pop | `pop<T>()`, `pop<bounded_array>()` |
| File I/O | `readfs(fd, off, whence)`, `writefs(fd, off, whence)` |
| Slicing | `slice(off_s, count)`, `slice(off_s)` |
| Access | `data()` |
| Comparison | `operator bool()` |

```cpp
// Parse ELF header from a memory-mapped file
stx::map_file elf{fd, file_size};

stx::u32 magic   = mapped.read<stx::u32>();      // 0x7F 'E' 'L' 'F'
stx::u8  ei_class = mapped.pop<stx::u8>();        // 1 = 32-bit, 2 = 64-bit
stx::u8  ei_data  = mapped.pop<stx::u8>();        // 1 = LE, 2 = BE
mapped.skip(stx::off_s{14});                      // skip to e_type
stx::u16 e_type   = mapped.read<stx::u16>();

// Re-read at offset without moving cursor
auto magic2 = mapped.read<stx::u32>(stx::off_s{0});
```

### Ownership

`map_file` is move-only (no copy constructor or copy assignment).

```cpp
map_file(const map_file&) = delete;
map_file& operator=(const map_file&) = delete;
```

```cpp
// Transfer ownership
stx::map_file moved = std::move(mapped);
```

