# `ct::record` — compile-time `key → type` descriptor

> Header: `#include <lbyte/stx/ct/record.hpp>` (or `#include <lbyte/stx/ct.hpp>`)
> Module: `import lbyte.stx.ct.record;` (or `import lbyte.stx.ct;`)

A `record` is a **layout descriptor** that works like a *struct*: certain keys exist
at compile time, queries (`offset_of`, `byte_total`, `value_of`, …) resolve with
`constexpr`, and binary reads redistribute the bytes member-by-member — exactly as
a real `struct` with the same alignment would.

`record` is an **empty type**: only the *value tuple* (`value_t`) occupies memory at
runtime, and only when you load it. Everything else is pure compile-time
computation — zero cost.

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

- Each **`member<Key, Type>`** binds a key (an `enum class` or `off_s`/`rva_s`)
  to a binary type (`binary_readable`).
- All keys must **share the same type** and be **unique**; all values must be
  **trivially copyable**. Enforced with `static_assert`.

## Layout (like a struct)

The default alignment follows the compiler's rules: each member is placed at its
naturally-aligned offset (same `alignof` / `offsetof` a real `struct` would use)
and the total includes trailing padding.

```cpp
static_assert( info::count == 3 );

static_assert( info::offsets       [0] == 0 );   // aligned offsets
static_assert( info::offsets       [1] == 2 );
static_assert( info::offsets       [2] == 4 );

static_assert( info::offset_of<sec::crc>() == 4 );   // key-level "offsetof"
static_assert( info::index_of<sec::stride>() == 1 );  // ordinal position

static_assert( info::byte_total == 8 );       // aligned total (trailing pad)
static_assert( info::packed_total == 6 );     // packed total (no pad)
static_assert( info::max_align == 4 );
```

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
or inside `if constexpr` — no runtime cost.

### Typed access by key ("map-like")

Given a loaded `value_t`, `get<Key>` returns the member **with its own type** —
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

All keys share one type, so the descriptor arrays are plain homogeneous arrays —
that is where `for` and `[]` work at runtime:

```cpp
// keys / sizes / meta are std::array of size count
for ( auto k : info::keys )      // "count", "stride", "crc"
    table_for[k] = nullptr;

for ( const auto& m : info::meta )   // {key, offset, size}
    std::printf( "%zu @ %zu (%zu bytes)\n", m.key, m.offset, m.size );

info::meta[1];                    // index by ordinal position
constexpr auto m = info::meta_of( sec::crc );   // by key, constexpr
static_assert( m.offset == 4 && m.size == 4 );
```

Those `for`/`[]` give you the **layout** (what is at each offset and how many
bytes), which is what can change at runtime. What a `for` *cannot* do is
re-type the loop variable per index — that requires reflection — so typed
member access goes through `visit`/`fold`/`std::get`.

## Reading and writing

`value_t` is the tuple of value types — it does *not* carry the record layout.

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
lets `ptr` and `memcur` read a record **member-by-member** — the same as
casting to your struct — through the generic `ptr_ops` mixin (no per-record
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

## Typical use cases

- **Layout descriptor**: offsets and sizes without reserving memory.
- **Change-order-proof lookups**: `has`/`value_of`/`index_of` by key, not by position.
- **Seeding tree offsets**: `offset_of<K>` + a runtime value.
- **Reading binary schemas/records**: `store`/`load`, or `pop<rec>` on a `memcur` —
  physical and logical order defined in one place.

## See also

- `record_value_of_t<Rec>` = `Rec::value_t`, useful for generic access.
- Include pieces: `<lbyte/stx/ct/str.hpp>` (CT strings) and
  `<lbyte/stx/ct/record.hpp>` (this); `<lbyte/stx/ct.hpp>` includes both.