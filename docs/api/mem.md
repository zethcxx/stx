# mem.hpp

## Free Functions (`stx::mem`)

### `read<Type>(addr, off)`
```cpp
template<binary_readable Type, address_like Addr, byte_offset OffT = off_s>
[[nodiscard]] Type mem::read(Addr base, OffT off = OffT{}) noexcept;
```
```cpp
u32 v = stx::mem::read<u32>(base, off_s{8});
```

### `read_le<Type>` / `read_be<Type>`
```cpp
template<byte_swappable Type, address_like Addr, byte_offset OffT = off_s>
[[nodiscard]] Type mem::read_le(Addr base, OffT off) noexcept;
template<byte_swappable Type, address_like Addr, byte_offset OffT = off_s>
[[nodiscard]] Type mem::read_be(Addr base, OffT off) noexcept;
```
```cpp
u32 le = stx::mem::read_le<u32>(base, off_s{0});
u32 be = stx::mem::read_be<u32>(base, off_s{4});
```

### `write<Type>(addr, off, value)`
```cpp
template<binary_readable Type, address_like Addr, byte_offset OffT = off_s>
void mem::write(Addr base, OffT off, const Type& value) noexcept;
```
```cpp
stx::mem::write<u32>(base, off_s{8}, 0xDEADBEEF);
```

### `read_raw<Type>` / `write_raw<Type>`
```cpp
template<binary_readable Type, byte_offset OffT = off_s>
[[nodiscard]] Type mem::read_raw(address_like auto base, OffT off = OffT{}) noexcept;

template<binary_readable Type, byte_offset OffT = off_s>
void mem::write_raw(address_like auto base, OffT off, Type value) noexcept;
```
```cpp
u32 v = stx::mem::read_raw<u32>(ptr, off_s{0});
stx::mem::write_raw(ptr, off_s{0}, u32{42});
```

---

## `ptr<T, Stride>`
```cpp
template<typename T = void, uptr Stride = 1>
class ptr {
    constexpr ptr() noexcept = default;
    constexpr explicit ptr(address_like auto addr) noexcept;
    [[nodiscard]] constexpr ptr(T* raw_ptr) noexcept;
    constexpr ptr(null_t) noexcept;

    // --- state ---
    [[nodiscard]] auto raw() noexcept -> T*;
    [[nodiscard]] auto raw() const noexcept -> const T*;
    [[nodiscard]] constexpr uptr addr() const noexcept;
    explicit operator bool() const noexcept;
    explicit operator uptr() const noexcept;

    // --- rebind ---
    ptr& operator=(T* raw_ptr) noexcept;
    constexpr ptr& operator=(address_like auto addr) noexcept;
    template<uptr OtherStride>
    constexpr ptr& operator=(const ptr<T, OtherStride>& other) noexcept;
    template<typename U> [[nodiscard]] constexpr ptr<U> as_p() const noexcept;

    // --- deref ---
    auto operator*() noexcept -> T&         requires (not std::is_void_v<T>);
    auto operator*() const noexcept -> const T& requires (not std::is_void_v<T>);
    auto operator->() noexcept -> T*        requires (not std::is_void_v<T>);
    auto operator->() const noexcept -> const T* requires (not std::is_void_v<T>);

    // --- navigation ---
    template<std::integral U>
    constexpr ptr_light<T, Stride> operator[](U offset) const noexcept;
    template<byte_offset OffT>
    constexpr ptr_light<T, Stride> operator[](OffT offset) const noexcept;

    // --- chain (read + return ptr) ---
    template<byte_offset OffT>
    auto operator>>(OffT offset) const noexcept -> ptr<T, Stride>
        requires (not std::is_void_v<T>);
    template<byte_offset OffT>
    auto operator>>(OffT offset) const noexcept -> ptr<void, Stride>
        requires (std::is_void_v<T>);

    // --- walk (byte chase, no stride) ---
    template<typename ReturnType = T, byte_offset OffT>
    auto walk(OffT offset) const noexcept -> ptr<ReturnType, Stride>;

    // --- stride ---
    template<uptr NewStride> constexpr ptr<T, NewStride> step() const noexcept;
    template<typename U>     constexpr ptr<T, sizeof(U)> step() const noexcept;

    // --- read ---
    template<typename U = T> auto read() const noexcept -> U;
    template<bounded_array U> auto read() const noexcept -> bounded_array_t<U>;
    template<typename U = T> auto read_p() const noexcept -> ptr<U>;
    template<typename U = T> auto read_le() const noexcept -> U;
    template<typename U = T> auto read_be() const noexcept -> U;
    template<bounded_array U> auto as_view() const noexcept -> std::span<const std::remove_all_extents_t<U>>;
    template<typename U = T> auto as_view(usize count) const noexcept -> std::span<const U>;
    template<writable_buffer R> void read_into(R&& buf) const noexcept;

    // --- pop ---
    template<typename U = T> auto pop() noexcept -> U;
    template<bounded_array U> auto pop() noexcept -> bounded_array_t<U>;
    template<writable_buffer R> ptr& pop_into(R&& buf) noexcept;

    // --- write ---
    template<typename U = T> void write(U value) const noexcept;
    template<contiguous_buffer R> void write(R&& range) const noexcept;
    template<byte_swappable U = T> void write_le(U value) const noexcept;
    template<byte_swappable U = T> void write_be(U value) const noexcept;

    // --- push ---
    template<typename U = T> ptr& push(const U& value) noexcept;
    template<contiguous_buffer R> ptr& push(R&& range) noexcept;

    // --- arithmetic ---
    template<byte_offset OffT> constexpr ptr operator+(OffT offset) const noexcept;
    template<byte_offset OffT> constexpr ptr operator-(OffT offset) const noexcept;
    template<byte_offset OffT> constexpr ptr& operator+=(OffT offset) noexcept;
    template<byte_offset OffT> constexpr ptr& operator-=(OffT offset) noexcept;
    constexpr off_s operator-(ptr other) const noexcept;
    template<address_like Addr> constexpr off_s diff(Addr other) const noexcept;

    // --- inc/dec ---
    constexpr ptr& operator++() noexcept;
    constexpr ptr  operator++(int) noexcept;
    constexpr ptr& operator--() noexcept;
    constexpr ptr  operator--(int) noexcept;

    // --- alignment ---
    template<std::unsigned_integral U = usize>
    constexpr ptr align_up(U alignment) const noexcept;
    template<std::unsigned_integral U = usize>
    constexpr ptr align_down(U alignment) const noexcept;
    template<typename U> constexpr bool is_aligned() const noexcept;
    template<usize Alignment> constexpr bool is_aligned() const noexcept;

    // --- call ---
    template<class Sig, class... Args>
    decltype(auto) call(Args&&... args) const;
    template<class Sig> auto caller() const noexcept;

    // --- swap ---
    constexpr void swap(ptr& other) noexcept;
    friend constexpr void swap(ptr& a, ptr& b) noexcept;
};
```
```cpp
stx::ptr<u32> base{0x140000000};

// Offset navigation
auto next = base[off_s{0x100}];           // ptr_light, no deref
u32 val = base[off_s{0x100}].read<u32>();
base[off_s{8}].write(u32{0x1234});

// Chain (read + return ptr)
auto target = (base[off_s{0x100}] >> off_s{0x10} >> off_s{0x20}).addr();

// Walk (byte-level, no stride)
uptr addr = base.walk(off_s{8}).addr();

// Pop
u32 a = base.pop<u32>();                  // advances address by sizeof(u32)

// Push
base.push(u32{0xDEADBEEF});

// Stride
auto tbl = base.step<4>();                // ptr<void, 4>
uptr entry = (tbl >> off_s{3} >> off_s{5}).addr();

// Call
int result = base.call<int(int, int)>(10, 20);

// Read/Write endian-aware
u32 le = base.read_le<u32>();
base.write_be(off_s{0}, u32{0xAABBCCDD});
```

## `ptr_light<T, Stride>`
```cpp
template<typename T, uptr Stride>
struct ptr_light : ptr<T, Stride> {
    using ptr<T, Stride>::ptr;
    template<typename U> ptr_light operator[](U ) const = delete; // no chaining
};
```
```cpp
base[0x100]                // OK: ptr_light
// base[0x100][0x20]       // ERROR: deleted
(base + off_s{0x100})[off_s{0x20}] // OK
```
