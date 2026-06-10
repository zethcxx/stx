# fs.hpp

## `fs::origin`
```cpp
enum class origin : u8 { begin, current, end };
using fs::origin; // also at stx::origin
```
```cpp
stx::fs::origin dir = stx::fs::origin::begin;
```

## `dirty_vector<Type>`
```cpp
template<binary_readable Type = u8>
using dirty_vector = std::vector<Type, details::vec_init_allocator<Type>>;
```
```cpp
stx::dirty_vector<stx::u32> buf(100); // no zero-init
```

## Stream Positioning

### `fs::setposfs` / `fs::skipfs`
```cpp
void fs::setposfs(std::istream& file, const off_s offset, const origin dir = origin::begin) noexcept;
void fs::skipfs(std::istream& file, const off_s offset) noexcept;
```
```cpp
stx::fs::setposfs(file, off_s{128});
stx::fs::skipfs(file, off_s{32});
```

### `fs::setposos` / `fs::skipos`
```cpp
void fs::setposos(std::ostream& file, const off_s offset, const origin dir = origin::begin) noexcept;
void fs::skipos(std::ostream& file, const off_s offset) noexcept;
```

## Stream Read

### `fs::readfs<Type>(istream, off_s, origin)`
```cpp
template<binary_readable Type> [[nodiscard]]
std::expected<Type, std::errc> fs::readfs(
    std::istream& file, const off_s offset = off_s{0}, const origin dir = origin::begin) noexcept;
```
```cpp
auto h = stx::fs::readfs<header>(file, off_s{0});
if (!h) return;
```

### `fs::readfs<Type>(istream, span<Type>, off_s, origin)`
```cpp
template<binary_readable Type>
std::expected<void, std::errc> fs::readfs(
    std::istream& file, std::span<Type> out, const off_s offset, const origin dir = origin::begin) noexcept;
```
```cpp
std::array<u32, 16> arr;
auto res = stx::fs::readfs(file, std::span{arr}, off_s{64});
```

### `fs::readfs<Type>(istream, off_s, count, origin)`
```cpp
template<binary_readable Type = u8> [[nodiscard]]
std::expected<dirty_vector<Type>, std::errc> fs::readfs(
    std::istream& file, const off_s offset, const usize count, const origin dir = origin::begin);
```
```cpp
auto bytes = stx::fs::readfs(file, off_s{0}, 256);
```

### `fs::readfs<Type, Size>(istream, off_s, origin)`
```cpp
template<binary_readable Type, usize Size> requires (Size > 0) [[nodiscard]]
std::expected<std::array<Type, Size>, std::errc> fs::readfs(
    std::istream& file, const off_s offset = off_s{0}, const origin dir = origin::begin) noexcept;
```
```cpp
auto block = stx::fs::readfs<u8, 64>(file, off_s{0});
```

## Stream Write

### `fs::writefs<Type>(ostream, off_s, const Type&)`
```cpp
template<binary_readable Type>
std::expected<void, std::errc> fs::writefs(
    std::ostream& file, const off_s offset, const Type& value) noexcept;
```
```cpp
auto r = stx::fs::writefs(file, off_s{0}, u32{0xDEADBEEF});
```

### `fs::writefs<Type>(ostream, off_s, span<const Type>)`
```cpp
template<binary_readable Type>
std::expected<void, std::errc> fs::writefs(
    std::ostream& file, const off_s offset, std::span<const Type> buf) noexcept;
```

## `map_flag`
```cpp
enum class map_flag : u8 {
    none = 0, write = 1<<0, exec = 1<<1,
    shared = 1<<2, priv = 1<<3, populate = 1<<4,
};
constexpr map_flag operator|(map_flag, map_flag) noexcept;
constexpr bool operator&(map_flag, map_flag) noexcept;
```
```cpp
auto flags = map_flag::write | map_flag::shared;
if (m.flags() & map_flag::write) {}
```

## `map_file`
```cpp
class map_file : private memcur<std::byte> {
    static auto open(const std::filesystem::path& path, map_flag flags = {}) noexcept
        -> std::expected<map_file, std::errc>;
    static auto open(const std::filesystem::path& path, off_s offset, usize size, map_flag flags = {}) noexcept
        -> std::expected<map_file, std::errc>;

    map_flag flags() const noexcept;
    bool     is_alive() const noexcept;
    void     swap(map_file& other) noexcept;
    auto     flush() noexcept -> std::expected<void, std::errc>;

    // Re-exported from memcur:
    // seek, skip, tell, remaining, pop, push, as_view,
    // read_into, pop_into, read_strvw, bytes, as_p,
    // operator bool, size, base
};
```
```cpp
auto m = map_file::open("data.bin", map_flag::write);
if (!m) return;

u32 val = m->as_p<u32>()[off_s{0x100}].read<u32>();
m->as_p<u32>()[off_s{0x104}].write(u32{0xDEADBEEF});

// Cursor
m->seek(off_s{0x100});
u32 a = m->pop<u32>();
u32 b = m->pop<u32>();

// Flush
m->flush();
```

## `memcur<ByteType>`
```cpp
template<buffer_type ByteType = std::byte>
class memcur {
    memcur() noexcept = default;
    memcur(memcur&&) noexcept;
    memcur& operator=(memcur&&) noexcept;

    memcur(std::span<ByteType> buf) noexcept;
    memcur(ptr<ByteType> base, usize size) noexcept;

    template<typename Ptr>
        requires std::is_pointer_v<Ptr> && std::convertible_to<Ptr, const void*>
    constexpr memcur(Ptr data, usize size) noexcept;

    // --- state ---
    explicit operator bool() const noexcept;
    usize size() const noexcept;
    uptr  base() const noexcept;
    std::span<const std::byte> bytes() const noexcept;
    std::span<std::byte> bytes() noexcept requires (!std::is_const_v<ByteType>);
    template<typename T = ByteType> constexpr ptr<T> as_p() const noexcept;

    // --- cursor ---
    void seek(off_s off, origin dir = origin::begin) noexcept;
    void skip(const off_s offset) noexcept;
    off_s tell() const noexcept;
    off_s remaining() const noexcept;

    // --- pop ---
    template<binary_readable T> T pop() noexcept;
    template<bounded_array U>   details::bounded_array_t<U> pop() noexcept;

    // --- push ---
    template<binary_readable T>
        requires (!contiguous_buffer<T> && !std::is_const_v<ByteType>)
    auto& push(const T& value) noexcept;
    template<contiguous_buffer R>
        requires (!std::is_const_v<ByteType>)
    auto& push(R&& buf) noexcept;

    // --- view ---
    template<bounded_array U>
    std::span<const std::remove_all_extents_t<U>> as_view() noexcept;
    template<binary_readable T>
    std::span<const T> as_view(usize count) noexcept;

    // --- read / pop into ---
    template<writable_buffer R> void  read_into(R&& buf) noexcept;
    template<writable_buffer R> auto& pop_into(R&& buf) noexcept;
    std::string_view read_strvw() noexcept;
    std::string_view read_strvw(usize max) noexcept;
};
```

### Deduction Guides
```cpp
template<buffer_type B>    memcur(B*, usize) -> memcur<B>;
template<typename T>
    requires (!buffer_type<T>)
    memcur(T*, usize) -> memcur<std::conditional_t<std::is_const_v<T>, const std::byte, std::byte>>;
```
```cpp
stx::memcur r{static_cast<std::byte*>(nullptr), 256};  // memcur<std::byte>
stx::memcur r{span};                                     // from std::span<ByteType>

// Cursor
r.seek(off_s{64});
u32 val = r.pop<u32>();
r.skip(off_s{4});
r.push(u32{0x12345678});

// View
auto sp = r.as_view<u32>(4);
```

## `fs::readfs` / `fs::writefs` for `map_file`
```cpp
template<binary_readable Type> [[nodiscard]]
std::expected<Type, std::errc> fs::readfs(const map_file& m, const off_s offset) noexcept;

template<binary_readable Type>
std::expected<void, std::errc> fs::writefs(map_file& m, const off_s offset, const Type& value) noexcept;
```
```cpp
u32 v = stx::fs::readfs<u32>(m, off_s{0x100});
stx::fs::writefs(m, off_s{0x104}, u32{0xDEADBEEF});
```

## `fs::readfs` / `fs::writefs` for `span`
```cpp
template<binary_readable Type> [[nodiscard]]
std::expected<Type, std::errc> fs::readfs(std::span<const std::byte> buf, const off_s offset) noexcept;

template<binary_readable Type>
std::expected<void, std::errc> fs::writefs(std::span<std::byte> buf, const off_s offset, const Type& value) noexcept;
```
