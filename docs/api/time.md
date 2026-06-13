# `time.hpp`

All examples assume `using namespace stx;` for brevity.

## Overview

`time.hpp` provides portable UNIX time utilities and a lightweight stopwatch
built on `std::chrono` under C++23.

Everything lives in `namespace stx::time`.

---

## Clock Aliases

| Alias                | Underlying Type                      | Purpose                            |
|----------------------|--------------------------------------|------------------------------------|
| `time::wall_clock`   | `std::chrono::system_clock`          | Wall-clock / UNIX epoch clock      |
| `time::hires_clock`  | `std::chrono::high_resolution_clock` | Highest-resolution clock available |
| `time::steady_clock` | `std::chrono::steady_clock`          | Monotonic clock (never adjusted)   |

```cpp
using wall_clock   = std::chrono::system_clock;
using hires_clock  = std::chrono::high_resolution_clock;
using steady_clock = std::chrono::steady_clock;
```

---

## UNIX Time Utilities (UTC)

Epoch: **1970-01-01 00:00:00 UTC**. All conversions go through `wall_clock`.

### `time::from_unix<Dur>(count)`

Creates a `wall_clock::time_point` from a UNIX timestamp.

```cpp
template<std::chrono::duration Dur = std::chrono::seconds>
[[nodiscard]] constexpr wall_clock::time_point from_unix(u64 count) noexcept;
```

```cpp
auto tp = time::from_unix(1'700'000'000);             // seconds
auto tp = time::from_unix<std::chrono::milliseconds>(millis);
```

### `time::to_unix<Dur>(tp)`

Extracts a UNIX timestamp from a `wall_clock::time_point`.

```cpp
template<std::chrono::duration Dur = std::chrono::seconds>
[[nodiscard]] constexpr u64 to_unix(wall_clock::time_point tp) noexcept;
```

```cpp
u64 sec = time::to_unix(tp);
u64 ms  = time::to_unix<std::chrono::milliseconds>(tp);
```

### Current UNIX time

```cpp
template<std::chrono::duration Dur = std::chrono::seconds>
[[nodiscard]] u64 now() noexcept;

[[nodiscard]] u64 now_ms() noexcept;       // shorthand for now<std::milli>
[[nodiscard]] u64 now_ns() noexcept;       // shorthand for now<std::nano>
```

```cpp
u64 s  = time::now();       // seconds
u64 ms = time::now_ms();    // milliseconds
u64 ns = time::now_ns();    // nanoseconds
```

---

## `time::stopwatch`

Lightweight elapsed-time utility based on `steady_clock` (monotonic).

```cpp
struct stopwatch
{
    template<class Duration = std::chrono::milliseconds>
    [[nodiscard]] Duration elapsed() const noexcept;

    template<class Duration = std::chrono::milliseconds>
    [[nodiscard]] Duration lap() noexcept;

    void reset() noexcept;
};
```

### Members

| Member         | Returns | Description                                       |
|----------------|---------|---------------------------------------------------|
| `elapsed<D>()` | `D`     | Time since construction or last `reset()`/`lap()` |
| `lap<D>()`     | `D`     | Elapsed since last lap, then resets the counter   |
| `reset()`      | `void`  | Restarts the timer                                |

All duration-accepting members default to `std::chrono::milliseconds`.

```cpp
time::stopwatch sw;

do_work();

auto ms = sw.elapsed();                           // default: milliseconds
auto ns = sw.elapsed<std::chrono::nanoseconds>(); // nanosecond precision

auto lap1 = sw.lap();                             // returns and resets
do_more_work();
auto lap2 = sw.lap();

sw.reset();
auto after_reset = sw.elapsed();                 // near zero
```

### Why `Duration` as return type?

`elapsed()` and `lap()` return `std::chrono::duration` (not a raw `u64`),
giving you full access to chrono's duration API:

```cpp
auto ms = sw.elapsed();

// compare directly
if (ms > 100ms) { /* slow */ }

// convert flexibly
auto us = std::chrono::duration_cast<std::chrono::microseconds>(ms);

// extract count when needed
u64 raw = ms.count();
```

---

## Portable Binary Format Converters

Convert raw integers read from binary data (via `ptr::read`, `memcur::pop`,
`fs::read`, etc.) into `wall_clock::time_point` and back.

| Function                                 | Input            | Description                            |
|------------------------------------------|------------------|----------------------------------------|
| `from_filetime(u64)` / `to_filetime(tp)` | Windows FILETIME | 100-ns intervals since 1601-01-01 UTC  |
| `from_dos(u32)` / `to_dos(tp)`           | DOS date/time    | Bit-packed 32-bit (FAT/ZIP/EXE)        |
| `from_ntp(u32)` / `to_ntp(tp)`           | NTP timestamp    | Seconds since 1900-01-01 UTC           |
| `from_ntp(u64)`                          | NTP 64-bit       | Seconds + fraction; fraction discarded |

### FILETIME (Windows)

```cpp
u64 raw_ft = cur.pop<u64>();                // e.g. from NTFS $MFT
auto tp    = time::from_filetime(raw_ft);
u64 ft     = time::to_filetime(tp);
```

### DOS date/time (FAT / ZIP / EXE)

```cpp
u32 raw_dos = cur.pop<u32>();               // bit-packed 32-bit
auto tp     = time::from_dos(raw_dos);
u32 dos     = time::to_dos(tp);
```

The DOS format has:
- **Date** (upper 16 bits): `year-1980` (7 bits) | `month` (4) | `day` (5)
- **Time** (lower 16 bits): `hours` (5) | `minutes` (6) | `seconds/2` (5)
- **Range**: 1980–2107, even seconds only (odd seconds truncated on round-trip)

### NTP timestamp

```cpp
u32 raw_ntp = cur.pop<u32>();               // seconds since 1900
auto tp     = time::from_ntp(raw_ntp);

u64 raw64   = cur.pop<u64>();               // upper 32 = seconds, lower 32 = fraction
auto tp2    = time::from_ntp(raw64);        // fraction discarded

u32 ntp     = time::to_ntp(tp);             // back to NTP seconds
```

NTP uses unsigned 32-bit seconds since 1900-01-01 (next wrap: ~2036). Pre-1970
timestamps are clamped to epoch (the `time_point` nanosecond range can't hold
them).

---

## Full Example

```cpp
auto tp   = time::from_unix(1'700'000'000);
u64 sec   = time::to_unix(tp);                      // 1700000000
u64 now_s = time::now();
u64 now_ms = time::now_ms();

time::stopwatch sw;
// ... work ...
auto elapsed = sw.elapsed();                        // milliseconds
auto lap     = sw.lap<std::chrono::nanoseconds>();  // nanosecond lap

// Reading from binary:
auto ft = cur.pop<u64>();                           // raw FILETIME
auto tp_ft = time::from_filetime(ft);               // → time_point

auto dos = cur.pop<u32>();                          // raw DOS datetime
auto tp_dos = time::from_dos(dos);                  // → time_point
```

