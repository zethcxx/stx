# `ct::record` - compile-time `key -> type` descriptor

> Header: `#include <lbyte/stx/ct/record.hpp>` (or `#include <lbyte/stx/ct.hpp>`)
> Module: `import lbyte.stx.ct.record;` (or `import lbyte.stx.ct;`)

A `record` is a **layout descriptor** that works like a *struct*: certain keys exist
at compile time, queries (`offset_of`, `byte_total`, `value_of`, ...) resolve with
`constexpr`, and binary reads redistribute the bytes member-by-member - exactly as
a real `struct` with the same alignment would.

`record` is an **empty type**: only the *value tuple* (`value_t`) occupies memory at
runtime, and only when you load it. Everything else is pure compile-time
computation - zero cost.

## Definition

```cpp
namespace ct = lbyte::stx::ct;

enum class sec : u8 { count, stride, crc };

using info = ct::record<
    ct::member<sec::count,  u16>,
    ct::member<sec::stride, u32>,
    ct::member<sec::crc,    u32>
>;
```

- Each **`member<Key, Type>`** binds a key (an `enum class`, an `off_s`/`rva_s`,
  or a string literal via `smember`) to a binary type (`binary_readable`).
- All keys must **share the same type** and be **unique**; all values must be
  **trivially copyable**. Enforced with `static_assert`.

## Layout (like a struct)

The default alignment follows the compiler's rules: each member is placed at its
naturally-aligned offset (same `alignof` / `offsetof` a real `struct` would use)
and the total includes trailing padding.

```cpp
static_assert( info::count == 3 );

static_assert( info::offsets        [0] == 0 );   // aligned offsets
static_assert( info::offsets        [1] == 2 );
static_assert( info::offsets        [2] == 4 );
static_assert( info::packed_offsets [2] == 4 );   // offsets if fully packed

static_assert( info::offset_of<sec::crc>() == 4 );   // key-level "offsetof"
static_assert( info::index_of<sec::stride>() == 1 );  // ordinal position

static_assert( info::byte_total == 8 );       // aligned total (trailing pad)
static_assert( info::packed_total == 6 );     // packed total (no pad)
static_assert( info::max_align == 4 );
```

- `offsets`   - aligned per-member offsets (`std::array<usize, count>`).
- `packed_offsets` - the same offsets as if every member were packed (the
  `attr::packed` layout, no member padding - but record-level `gap`/`align`
  attributes still apply).
- `byte_total` / `packed_total` - totals for those two layouts.
- `member_max_align` / `trailing_align` / `max_align` - the per-member maximum
  alignment, the alignment used for trailing padding, and the record's reported
  alignment (rises to `attr::align<N>` if given).

### Attributes

| Attribute           | Effect                                                              |
| ------------------- | ------------------------------------------------------------------- |
| `attr::packed`      | Packed layout (no padding), like `#pragma pack`                     |
| `attr::align<N>`    | Align to `N` (per-member or whole record)                           |
| `attr::gap<N>`      | Insert `N` bytes of padding before the next member                  |

Attributes go **on the member** or **at the record level**:

```cpp
using header = ct::record<
    ct::attr::packed,                            // pack the whole record
    ct::member<sec::count, u16>,                 // offset 0, 2 bytes
    ct::attr::gap<6>,                            // 6 bytes of filler
    ct::member<sec::crc, u32, ct::attr::align<8>> // aligned to 8
>;

// packed: no padding except the gap and explicit align
```

### Aligning the whole record

Put `attr::align<N>` at the top of the list and the record behaves like an
over-aligned `struct aligment(N)`: the reported alignment (`max_align`) rises
to `N` and the total gets **trailing padding up to `N`**, while each member
keeps its own natural offset:

```cpp
using r = ct::record<
    ct::attr::align<16>,                         // whole record aligned to 16
    ct::member<sec::a, u8>,
    ct::member<sec::b, u16>,
    ct::member<sec::c, u32>
>;

static_assert( r::offsets[0] == 0 );        // members keep natural offsets
static_assert( r::offsets[1] == 2 );
static_assert( r::offsets[2] == 4 );
static_assert( r::byte_total   == 16 );     // trailing padding up to N
static_assert( r::packed_total == 7 );      // fully packed would be 7 bytes
static_assert( r::max_align    == 16 );     // whole record reported align
```

Combine both levels freely: `align<N>` on a member forces *that* member's
offset, `attr::packed` at the top disables all auto padding (kept only where a
member or gap explicitly asks for it).

## Queries

```cpp
static_assert( info::has<sec::crc> );                          // key exists
static_assert( std::same_as<info::value_of<sec::stride>, u32> ); // key's type
static_assert( info::key_of<u32>() == sec::stride );           // key by type
```

All are `constexpr`, usable in `static_assert`, as a template argument,
or inside `if constexpr` - no runtime cost.

### Typed access by key ("map-like")

Given a loaded `value_t`, `get<Key>` returns the member **with its own type** -
the analogue of `rec[key]` / `rec.field`:

```cpp
info::value_t val{ 1, 0x1000, 0xDEADBEEF };

auto crc = info::get<sec::crc>(val);      // u32 = 0xDEADBEEF, keyed
info::get<sec::stride>(val) = 0x2000;     // lvalue: supports writes
```

> Literal `record[key]` via `operator[]` is *not expressible* in C++23: the return
> type cannot depend on the *value* of the key (reflection would be needed).
> `get<Key>` achieves the same because the key travels as a template argument
> (`index_of<Key>()` resolves the index at compile time). To dispatch by a
> **runtime** key, use the homogeneous descriptor tables `meta`/`meta_of` below.

## String keys (`smember`)

Instead of an enum, a key can be a string literal carried by the compile-time
type `key_str<N>` (default capacity 32) via `ct::smember<"name", Type>`:

```cpp
using header = ct::record<
    ct::smember<"count", u16>,
    ct::smember<"stride", u32>,
    ct::smember<"crc",   u32>
>;
```

Every `smember` key has the same C++ type (`key_str<32>`), so the homogeneity
rules hold unchanged and the layout is **identical** to the enum-keyed twin.
Queries and `get` accept a string literal directly; `value_of`/`has` take a
`key_str` value:

```cpp
static_assert( header::index_of<"crc">()  == 2 );
static_assert( header::offset_of<"crc">() == 4 );
static_assert( header::key_of<u32>()      == ct::key_str<32>{ "crc" } );

header::value_t val{ 1, 0x1000, 0xDEADBEEF };
auto crc  = header::get<"crc">( val );          // u32
header::get<"stride">( val ) = 0x2000;

static_assert( not header::has<ct::key_str<32>{ "nope" }> );  // unknown key
static_assert( std::same_as<header::value_of<ct::key_str<32>{ "count" }>, u16> );
```

> String keys traded compile-time rename checking for readability: a typo (or a
> rename) in a literal is a compile error at *use* (the `static_assert`s above),
> but your **editor/LSP does not track renames** the way it tracks `enum`
> members. Prefer `smember` for stable or one-off schemas and `enum` keys for
> schemas that evolve.

## Iterating members

Because each member has a **distinct type**, *typed* iteration uses `visit`/`fold`
(compile-time), not a runtime `for`:

```cpp
// visit: one call per member, in order
info::visit([](auto m) {
    // 'm' is the ct::member<K,T>; use typename decltype(m)::value_type, m.key...
    store( m.key );
});

// fold: accumulate in order
constexpr auto total = info::fold(usize{0}, [](auto m, usize acc) {
    return acc + sizeof(typename decltype(m)::value_type);
});
static_assert( total == 6 );
```

`info::members` exposes the heterogeneous member tuple for `std::get` /
structured bindings:

```cpp
auto&& [c, s, r] = info::members;
std::apply(...);   // std::get<T>/get<I> over the tuple
```

### Homogeneous descriptor tables: `for (...)`, `[]` and `meta_of`

All keys share one type, so the descriptor arrays are plain homogeneous arrays -
that is where `for` and `[]` work at runtime:

```cpp
// keys / sizes / meta are std::array of size count
for ( auto k : info::keys )      // "count", "stride", "crc"
    table_for[k] = nullptr;

for ( auto s : info::sizes )     // 2, 4, 4  -> sizeof(each value type)
    total += s;

for ( const auto& m : info::meta )   // {key, offset, size}
    std::printf( "%zu @ %zu (%zu bytes)\n", m.key, m.offset, m.size );

info::meta[1];                    // index by ordinal position
info::sizes[1];                   // same ordinal as keys/meta
constexpr auto m = info::meta_of( sec::crc );   // by key, constexpr
static_assert( m.offset == 4 && m.size == 4 );
```

- `keys`  - the member keys (`std::array<key_type, count>`).
- `sizes` - `sizeof` of each member's value type, in order (`std::array<usize, count>`).
- `meta`  - `{ key, offset, size }` per member (`std::array<member_meta, count>`).
- `meta_of(key)` - the same one entry, looked up **by key** (`const member_meta&`).

### Lookup semantics (compile-time vs runtime)

Lookups split into two families:

- **Compile-time key queries** - `has`, `value_of`, `index_of`, `offset_of`,
  `key_of`, `get`: a `constexpr` fold over the member pack. The result is a
  constant, so there is **zero runtime cost** regardless of `count`.
- **Runtime lookup** - `meta_of(key)`: a **linear scan** comparing keys by
  *value* (content equality - the same comparison works for `enum`, byte-offset
  newtypes and `key_str` strings alike). That is O(count) whenever the key is
  only known at runtime (e.g. a loop variable). `count` is fixed at compile
  time and the function is `constexpr`, so the scan is usually fully unrolled by
  the optimizer into a branch chain.

No compile-time hash table or sorted index is built: the arrays above are only
ever walked linearly. That trade-off is deliberate - it keeps *every* key kind
compareable by plain equality and avoids collision machinery.

When you know the key is a **contiguous `enum class`** starting at 0 (or you have
the ordinal another way), skip `meta_of` entirely and index the arrays directly -
`keys[i]` / `sizes[i]` / `meta[i]` are O(1). For a contiguous enum the ordinal is
just `to_underlying(key)`:

```cpp
// sec { count=0, stride=1, crc=2 } is contiguous
static constexpr usize crc_size = info::sizes[ to_underlying(sec::crc) ];  // 4

usize total = 0;
for ( usize i = 0; i < info::count; ++i )
    total += info::sizes[i];                               // O(count) once, no per-key scan
```

Prefer `meta_of` when the keys are sparse/non-contiguous or reorders/renames are
expected, so no code assumes `ordinal == to_underlying(key)`.

Those `for`/`[]` give you the **layout** (what is at each offset and how many
bytes), which is what can change at runtime. What a `for` *cannot* do is
re-type the loop variable per index - that requires reflection - so typed
member access goes through `visit`/`fold`/`std::get`.

## Reading and writing

`value_t` is the tuple of value types - it does *not* carry the record layout.

```cpp
info::value_t values{ 1, 0x1000, 0xDEADBEEF };

std::byte raw[info::byte_total]{};
ct::store<info>( std::span<std::byte>(raw), values );   // writes at offsets

auto back = ct::load<info>( std::span<const std::byte>(raw) );
auto [count, stride, crc] = back;
```

`ct::store<info>(std::byte*, values)` and `ct::store<info>(span, values)`
behave identically.

## As a struct for `ptr` / `memcur`

The record's `reader` fingerprint (see `details::record_like` in `mem.hpp`)
lets `ptr` and `memcur` read a record **member-by-member** - the same as
casting to your struct - through the generic `ptr_ops` mixin (no per-record
specialization in `mem`/`io`):

```cpp
ptr<std::byte> p{ raw };

auto first  = p.read<info>();  // returns value_t (no advance)
auto second = p.pop <info>();  // value_t and advances byte_total

assert( p.addr() == reinterpret_cast<uptr>(raw) + info::byte_total );

memcur cur{ raw, sizeof(raw) };
auto [count, stride, crc] = cur.pop<info>();  // same member-wise layout
```

This respects alignment, packing and gaps identically to `ct::load`/`ct::store`.

## Zero-copy views (`record_view`)

`ct::record_view<Rec>` is an **unowned window** over the raw bytes a record
describes. It never copies: `get<Key>()` returns a reference *into* the buffer
(mutating it changes the buffer) and iteration walks the constexpr member table
(key / offset / size) without touching a single byte.

```cpp
std::byte  raw[header::byte_total]{};
ct::record_view<header> r{ raw };            // or ct::view<header>(ptr)

r.get<"crc">()    = 0xCAFEBABEu;             // writes land in raw
r.get<"stride">() = 0x1000;
r.at<0>()         = 7;                       // by ordinal index

for ( auto m : r )                           // { key, offset, size }
    std::printf( "%s @ %zu (%zu bytes)\n", m.key.c_str(), m.offset, m.size );

static_assert( ct::record_view<header>::count()      == 3 );
static_assert( ct::record_view<header>::byte_count() == header::byte_total );
```

Construct from a `void*` (or a raw `uptr` address - e.g. device/section space);
a `const` view yields `const` references. The view reads/writes the same
member-wise offsets as `ct::store`/`ct::load`, so a view and a loaded `value_t`
never disagree about where a field lives.

## Typical use cases

- **Layout descriptor**: offsets and sizes without reserving memory.
- **Change-order-proof lookups**: `has`/`value_of`/`index_of` by key, not by position.
- **Seeding tree offsets**: `offset_of<K>` + a runtime value.
- **Reading binary schemas/records**: `store`/`load`, or `pop<rec>` on a `memcur` -
  physical and logical order defined in one place.

## See also

- `record_value_of_t<Rec>` = `Rec::value_t`, useful for generic access.
- Include pieces: `<lbyte/stx/ct/str.hpp>` (CT strings) and
  `<lbyte/stx/ct/record.hpp>` (this); `<lbyte/stx/ct.hpp>` includes both.