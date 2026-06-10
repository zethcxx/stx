# fs.hpp

## `origin`

Seek origin enum for `readfs` / `writefs`.

```cpp
enum class origin : u8
{
    begin   = SEEK_SET,
    current = SEEK_CUR,
    end     = SEEK_END
};
```

---

## `memcur<ByteType>`

A non-owning cursor over a contiguous range of byte-like elements (`ByteType` must satisfy `buffer_type`). Provides sequential reads/writes with stride-aware position tracking.

### Construction

```cpp
template<buffer_type ByteType>
class memcur
{
    memcur() noexcept;
    memcur(ByteType* p, usize size_in_elements) noexcept;
    template<address_like Ptr> memcur(Ptr p, usize len) noexcept;
    memcur(const memcur&) noexcept;
    memcur(memcur&&) noexcept;
};
```

```cpp
stx::memcur<std::byte> cur;               // default (null)
stx::memcur cur{data, len};               // CTAD -> memcur<std::byte>
stx::memcur<char> ccur{chars, 32};        // char-typed cursor
```

### Assignment

```cpp
memcur& operator=(const memcur&) noexcept;
memcur& operator=(memcur&&) noexcept;
```

### Position Queries

```cpp
[[nodiscard]] usize offset()  const noexcept; // element offset from start
[[nodiscard]] usize remain()  const noexcept; // remaining elements
[[nodiscard]] usize position() const noexcept; // byte position
[[nodiscard]] usize bytes_remain() const noexcept; // remaining bytes
```

```cpp
u32 buf[4];
stx::memcur cur{buf, 4};       // offset=0, remain=4, position=0
cur.skip(sizeof(buf) / 2);     // offset=2, position=8
usize rem = cur.remain();      // 2
```

### Cursor Movement

```cpp
void seek(off_s off)    noexcept; // absolute byte offset
void skip(off_s off)    noexcept; // relative byte offset
void skip(isize n)      noexcept; // element-level skip (respects stride)
void reset()            noexcept; // back to start
```

```cpp
cur.seek(off_s{16});  // jump to byte 16
cur.skip(off_s{-8});  // back 8 bytes
cur.skip(3);          // forward 3 elements
cur.reset();          // back to 0
```

### Read / Write

```cpp
template<binary_readable T> T        read(off_s off = {})  const;
template<binary_readable T> void     write(const T& value, off_s off = {});
template<binary_readable T> T        pop();

template<bounded_array T> auto       read(off_s off = {})  const;
template<bounded_array T> void       write(const T& value, off_s off = {});
template<bounded_array T> auto       pop();

void   read(void* dest, usize count, off_s off = {}) const;
void   write(const void* src, usize count, off_s off = {});
```

```cpp
auto v = cur.read<u32>(off_s{0});   // read u32 at byte 0
auto arr = cur.read<u8[4]>();       // read 4 bytes into array
cur.write<u32>(42);                 // write u32 at current pos
cur.pop<u32>();                     // read u32 + advance
```

### File I/O

```cpp
usize readfs(int fd, off_s off, origin whence = origin::begin);
usize writefs(int fd, off_s off, origin whence = origin::begin);
```

```cpp
auto n = cur.readfs(fd, off_s{0});         // read from fd at offset 0
cur.writefs(fd, off_s{-32}, origin::end);  // write to fd 32 bytes before EOF
```

### Slicing / Resizing

```cpp
memcur  slice(off_s off, usize count)      const;
memcur  slice(off_s off)                   const; // to end
void    resize(usize sz);
void    drop(usize n);
```

```cpp
auto sub = cur.slice(off_s{8}, 4);  // 4 elements starting at byte 8
auto tail = cur.slice(off_s{32});   // everything from byte 32
cur.drop(4);                        // discard first 4 elements
```

### Accessors

```cpp
[[nodiscard]] ByteType* data()               const noexcept;
[[nodiscard]] uptr      addr()               const noexcept;
template<address_like A> auto start_at(A addr) const;
```

```cpp
auto d = cur.data();           // raw pointer
auto a = cur.addr();           // address as uptr
auto c = cur.start_at(off_s{8}); // new memcur shifted by 8 bytes
```

### Comparison

```cpp
[[nodiscard]] explicit operator bool() const noexcept;
bool operator==(const memcur&) const noexcept;
```

```cpp
if (cur) { /* non-null */ }
```

---

## `map_file`

Memory-maps a file using OS primitives (`mmap` / `VirtualAlloc`). Inherits privately from `memcur<std::byte>` with select members re-exported.

### Construction

```cpp
map_file(int fd, usize length, off_s offset = off_s{});
map_file(map_file&&) noexcept;
```

```cpp
int fd = open("file.bin", O_RDONLY);
stx::map_file mapped{fd, 4096};     // map first 4096 bytes
```

### Move Assignment / Destruction

```cpp
map_file& operator=(map_file&&) noexcept;
~map_file() noexcept;
```

### Members (re-exported from `memcur<std::byte>`)

```cpp
    // Position
    usize offset() const noexcept;
    usize remain() const noexcept;

    // Movement
    void seek(off_s off) noexcept;
    void skip(off_s off) noexcept;

    // Read
    template<binary_readable T> T read(off_s off = {}) const;
    template<bounded_array T> auto read(off_s off = {}) const;
    void read(void* dest, usize count) const;

    // Write
    template<binary_readable T> void write(const T& value, off_s off = {});
    template<bounded_array T> void write(const T& value, off_s off = {});
    void write(const void* src, usize count);

    // Pop
    template<binary_readable T> T pop();
    template<bounded_array T> auto pop();

    // I/O
    usize readfs(int fd, off_s off, origin whence = origin::begin);
    usize writefs(int fd, off_s off, origin whence = origin::begin);

    // Slicing
    memcur<std::byte> slice(off_s off, usize count) const;
    memcur<std::byte> slice(off_s off) const;

    // Access
    auto data() const noexcept;

    // Comparison
    explicit operator bool() const noexcept;
```

```cpp
auto val = mapped.read<u32>();    // read u32 at cursor
mapped.seek(off_s{0});            // rewind
auto arr = mapped.read<u8[16]>(); // read 16 bytes into array
```

### Non-copyable

```cpp
map_file(const map_file&) = delete;
map_file& operator=(const map_file&) = delete;
```
