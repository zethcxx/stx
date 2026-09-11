# cycle.hpp

## Overview

`cycle.hpp` implements a zero-overhead, constexpr-friendly range cycler for C++23.

It supports:

- Infinite repetition (`cycle(r)`) - terminates with `break` or composition
- Bounded repetition (`cycle(r, n)`) - exactly `n` passes
- Interop with `stx::range` / `irange` and standard containers (`vector`, `span`, `array`, `string`)
- Sentinel-based iteration, models `std::ranges::input_range` / `view` and composes with `std::views` adaptors (`views::zip`, ...)
- No dynamic allocation, no vtable, all `constexpr`

Nothing in the C++ standard (C++23 or C++26) provides this facility; the proposal for standard `views::cycle` is targeted at C++29.

---

## Public API

### `cycle(r)` - infinite

```cpp
template<cycleable R> constexpr auto cycle( R&& r ) noexcept;
```

Repeats the range endlessly. `end()` is a sentinel that is never reached - stop with `break` or by composing with a bounded count.

```cpp
for (auto c : stx::cycle(palette))
{
    if (finished)
        break;
    set_color(c);
}
```

### `cycle(r, n)` - bounded

```cpp
template<cycleable R> constexpr auto cycle( R&& r, usize passes ) noexcept;
```

Yields the underlying elements exactly `n` times. Usable in a plain range-for with a known endpoint:

```cpp
for (auto i : stx::cycle(stx::range(0, 3), 2))
    process(i);   // 0 1 2 0 1 2
```

| Call              | Result        |
| ----------------- | ------------- |
| `cycle(r, 2)`     | `r` twice     |
| `cycle(r, 1)`     | single pass   |
| `cycle(r, 0)`     | empty         |
| `cycle(empty, n)` | empty         |
| `cycle(empty)`    | empty (no UB) |

---

## `cycleable`

```cpp
template<typename R>
concept cycleable
    = requires ( R&& r )
    {
        r.begin();
        r.end();
    };
```

Any range-like type with `begin()` / `end()`. The iteration machinery only needs:

- `*it` - dereference
- `++it` - advance
- `it == end` - pass completion (works with sentinels, e.g. `range_iter` vs `range_sentinel`)

This is why the same `cycle` works for `stx::range` and for `std::vector` with no special cases.

---

## Lifetime

Iterators are copied out of the underlying range at construction:

- `stx::range` / `irange` return value-carrying iterators, so `cycle(range(...))` and even temporaries are safe.
- Container iterators reference the container - the caller must keep it alive while iterating (same contract as standard views).

---

## Example Usage

### 1. Round-robin over a region

```cpp
for (auto off : stx::cycle(stx::range<stx::off_s>(0, 0x100, 0x10), 4))
    probe(off);   // 0x00 0x10 ... 0xf0, repeated 4x
```

### 2. Palette / spinner

```cpp
constexpr std::array colors{'|', '/', '-', '\\'};
for (auto c : stx::cycle(colors))
{
    draw(c);
    if (done)
        break;
}
```

### 3. Test repetitions

```cpp
auto pattern = std::vector<u8>{0xAA, 0x55};
for (auto b : stx::cycle(pattern, 3))
    write_byte(addr++, b);   // AA 55 AA 55 AA 55
```

### 4. Compile-time

```cpp
constexpr auto sum = [] {
    auto s = 0;
    for (auto i : stx::cycle(stx::range(0, 3), 3))
        s += i;
    return s;
}();
static_assert(sum == 9);
```

---

# Internal Design

## `details::cycle_sentinel`

```cpp
struct cycle_sentinel {};
```

Empty tag; an iterator compares equal when the cycle is exhausted.

## `details::cycle_iter`

```cpp
template<typename It, typename End>
struct cycle_iter
{
    using iterator_concept  = std::input_iterator_tag;
    using iterator_category = std::input_iterator_tag;
    using value_type        = std::iter_value_t<It>;
    using reference         = std::iter_reference_t<It>;
    using difference_type   = std::iter_difference_t<It>;

    It    first_      ;
    It    cur_        ;
    End   end_        ;
    usize passes_left_;

    constexpr decltype(auto) operator*() const noexcept;
    constexpr cycle_iter&    operator++() noexcept;
    constexpr cycle_iter     operator++(int) noexcept;
    // hidden friends: i == s, s == i, i != s, s != i
};
```

- `passes_left_ == usize(-1)` encodes **infinite** - on wrap the iterator resets to `first_` forever.
- On wrap with `passes_left_ > 1`, the counter decrements and `cur_` resets to `first_`.
- On the final wrap, `passes_left_` is set to `0`, making the iterator compare equal to the sentinel.
- `operator*` returns `decltype(auto)`: by value for `stx::range` iterators, by reference for container iterators.

## `details::cycle_view`

```cpp
template<typename It, typename End>
struct cycle_view
{
    It    first_      ;
    End   end_        ;
    usize passes_left_;

    constexpr iter_t begin() const noexcept;
    constexpr cycle_sentinel end() const noexcept;
};
```

`begin()` yields an iterator already positioned at `first_`; for an empty range (`first_ == end_`) or `passes == 0` it returns an exhausted iterator so the loop body never runs.

---

# Why cycle?

| Aspect        | Vanilla C++                                                    | stx                                         |
| ------------- | -------------------------------------------------------------- | ------------------------------------------- |
| Infinite      | Manual `for` + index reset with `%` and awkward sentinel logic | `cycle(r)` - declarative, `break` to stop   |
| Bounded       | `for (int k = 0; k < n; ++k) for (auto& x : r) ...` - nesting  | `cycle(r, n)` - single loop, no nesting     |
| Range interop | `%` needs `size()` / indexing - fails for sentinel ranges      | Iterator-based wrap works with `stx::range` |
| Constexpr     | Manual counters fine but verbose                               | Same, less boilerplate                      |

# Design Characteristics

- C++23 constexpr-friendly
- No dynamic allocation
- Models `std::ranges::input_range` / `view` - composes with `std::views` adaptors
- Sentinel-based iteration (works with sentinel-terminated ranges)
- Works with `stx::range` and standard containers
- Empty ranges are safe (empty cycle, no UB)
- Header-only
- Zero abstraction overhead
