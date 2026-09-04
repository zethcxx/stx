# arr.hpp

All examples assume `using namespace lbyte::stx;` for brevity.

## `arr<T, N>` — Modern fixed-size array

A contiguously-stored, zero-cost fixed-size array — a modern, std::array-compatible
refit that is also **indexable by enum** and by **byte-offset newtype**.
It holds a `std::array<T, N>` member (staying an aggregate so brace-init and
`constexpr` work in all compilers), giving the same layout and performance as
`std::array` while extending the API.

```cpp
template<typename Type, usize N> struct arr;
```

## Construction

Array construction mirrors `std::array`: explicit size, brace-init with CTAD,
and the `arr_of` factory.

```cpp
arr<int, 3>  a{ 1, 2, 3 };   // explicit
arr          b{ 1, 2, 3 };   // CTAD -> arr<int, 3>
arr<off_t,4> c{};            // value-initialized (matches legacy off_t)
```

### `arr_of<T>({...})` — std::to_array style factory

```cpp
template<typename Type, usize N> constexpr arr<Type, N> arr_of( Type (&&)[N] );
template<typename Type, usize N> constexpr arr<Type, N> arr_of( const Type (&)[N] ); // lvalue C-array

auto offs = arr_of<off_t>({ 0x00, 0x40, 0x80, 0xC0 });  // arr<off_t, 4>
auto vals = arr_of({ 7, 8, 9 });                        // arr<int, 3> (deduced)
```

The rvalue overload handles a brace array in a single call; the lvalue overload
accepts a **named C-array**, which is what lets C array-designators seed a typed
`arr` (see "Seeding an indexed table" below).

## Seeding an indexed table

Use `arr` with an `enum class` index as a typed, compile-time lookup table. Two
equivalent idioms, both evaluated entirely at compile time:

### 1. Constexpr IIFE (one expression, warning-free)

```cpp
enum class kind : u8 { small, medium, large, huge, count };
constexpr usize kKinds = 5;

inline constexpr auto limit = [] {
    arr<u64, kKinds> a{};
    using enum kind;
    a[small ] = 8;
    a[medium] = 64;
    a[large ] = 512;
    a[huge  ] = 4096;
    return a;
}();                                  // -> arr<u64, kKinds>
```

A *Immediately-Invoked Function Expression*: the anonymous lambda runs at the
`()` and its result is `limit`. `using enum kind` brings the enumerators into
scope, so no casts are needed. Single declaration, no warnings.

### 2. C-array designators + `arr_of` (keeps `[kind::X] = v` literal)

Array designators `[N] = v` are only valid in a C-array (not in a class), so
they live in a typed C-array and are lifted by `arr_of`:

```cpp
inline constexpr u64 raw[kKinds] = {
    [(u32)kind::small ] = 8,
    [(u32)kind::medium] = 64,
    [(u32)kind::large ] = 512,
    [(u32)kind::huge  ] = 4096,
};
inline constexpr auto limit = arr_of(raw);   // arr<u64, kKinds>
```

This preserves the exact `[kind::X] = v` notation. It needs the C-array `raw`
(compiler emits a C99-designator warning) and a cast on each index.

Both produce an `arr<u64, kKinds>` that is indexed by enum and usable as a
compile-time constant:

```cpp
static_assert( limit[kind::large] == 512 );   // compile-time lookup
u64 budget = limit[kind::medium];             // runtime friendly read
```

## Indexing

### By integral (raw)

Same as `std::array::operator[]` — no bounds check, zero-cost:

```cpp
arr<int, 4> a{ 1, 2, 3, 4 };
a[0];          // 1
a[3];          // 4
```

### By enum class (index key)

Any `enum class` can be used directly as an index; the value is unwrapped to its
underlying integer at compile time (zero-cost). This lets you index tables
semantically instead of scattering magic numbers:

```cpp
enum class sec : u8 { arena, stream, font, glyph };

arr<off_t, 4> off{ 0x00, 0x40, 0x80, 0xC0 };
arr<usize, 4> cnt{ 1, 2, 3, 4 };

auto at = [&](sec s) -> pair<off_t, usize> {
    return { off[s], cnt[s] };   // semantic, no cast, no magic numbers
};

let [aoff, acnt] = at(sec::arena);   // { 0x00, 1 }
```

The enum stays a compact distinct type (`sec : u8` above) — it never "decays" to
an integral, and the array index is resolved at compile time.

### By byte-offset newtype

Byte-offset `newtype`s (see `offset_s`, `is_offset_tag`, `byte_offset`) are also
valid index keys. A byte-offset key is treated as an **element index** — exactly
like an integral — never as a byte displacement:

```cpp
arr<u64, 8> cache{};
cache[off_s{4}] = 42;       // element at index 4 (the fifth u64)
```

Because the key is an element index, `sizeof(T)` is *not* factored in:

```cpp
arr<u64, 4> w{ 10, 20, 30, 40 };
w[off_s{2}] = 99;           // third u64 (== w[2]), NOT bytes 16..23
```

The one case where index and byte offset coincide is byte-sized elements
(`sizeof(T) == 1`, e.g. `arr<u8, N>`), so an `off_s` there reads naturally as a
real byte offset:

```cpp
arr<u8, 8> bytes{};
bytes[off_s{2}] = 0xFF;     // element 2 == byte 2
```

## Accessors and capacity

| Member | Notes |
|--------|-------|
| `a[i]` | raw index (no check) — integral or enum/offset key |
| `a.at(i)` | bounds-checked, throws `std::out_of_range` on failure |
| `a.front()` / `a.back()` | first / last element |
| `a.data()` | pointer to contiguous buffer |
| `a.size()` | number of elements |
| `a.max_size()` | max elements |
| `a.is_empty()` | true if `N == 0` (renamed from `empty`) |
| `a.fill(v)` | assign `v` to all elements |
| `a.swap(o)` | swap contents |

```cpp
arr<usize, 4> v{ 10, 20, 30, 40 };
v.size();         // 4
v.is_empty();     // false
v.front();        // 10
v.back();         // 40
v.at(2);          // 30
```

## Iterators and STL compatibility

Full iterator support via `begin`/`end`, `rbegin`/`rend`, `cbegin`/`cend`,
`crbegin`/`crend`. `arr` is a `std::contiguous_range` / `std::random_access_range`
and interoperates with `std::span`, algorithms, and `std::ranges`:

```cpp
arr<int, 4> a{ 1, 2, 3, 4 };

for (int x : a) {}            // range-for
std::span<int> sp(a);         // contiguous view
a == arr<int, 4>{ 1, 2, 3, 4 };  // defaulted ==
a <  arr<int, 4>{ 2, 0, 0, 0 };  // defaulted <=>
```

## Design notes

- **Zero-cost**: same layout and performance as `std::array`; enum/offset
  indexing is resolved at compile time with no runtime overhead.
- **Aggregate**: `arr` stays an aggregate (holds its `std::array` as a member),
  so `constexpr`, brace-init, and value-initialization work portably
  (inheriting `std::array` would break aggregate-initialization on all current
  major compilers).
- **No exceptions by default**: `operator[]` is unchecked (like `std::array`);
  use `at()` for checked access. An `std::expected`-based checked accessor is
  anticipated for C++26.
- **Extensible**: the enum/offset key mechanism mirrors the `byte_offset` /
  `is_offset_tag` extension points in `core.hpp`, so custom index keys can be
  registered the same way.
