# time.hpp

## Overview

`time.hpp` provides portable UNIX time utilities and high-resolution measurement tools built on top of `std::chrono` under C++23.

It standardizes:

- Conversion between UNIX timestamps and `std::chrono::time_point`.
- Retrieval of current UNIX time in seconds or milliseconds.
- A lightweight high-resolution stopwatch utility.

All utilities are header-only and ABI-transparent.

---

## Namespace: `stx`

### Clock Aliases

| Alias       | Underlying Type                      | Purpose                              |
|------------|---------------------------------------|--------------------------------------|
| `sys_clock`| `std::chrono::system_clock`           | Wall-clock / UNIX epoch clock        |
| `hr_clock` | `std::chrono::high_resolution_clock`  | High-resolution monotonic clock      |

```cpp
using sys_clock = std::chrono::system_clock;
using hr_clock  = std::chrono::high_resolution_clock;
```

---

## UNIX Time Utilities (UTC)

Epoch: **1970-01-01 00:00:00 UTC**

All conversions are defined relative to `system_clock`.

### Construction from UNIX timestamps

| Function | Description |
|----------|-------------|
| `from_unix_seconds(u64)` | Creates `time_point` from seconds since epoch |
| `from_unix_millis(u64)`  | Creates `time_point` from milliseconds since epoch |

```cpp
[[nodiscard]] inline constexpr sys_clock::time_point
from_unix_seconds(u64 seconds) noexcept;

[[nodiscard]] inline constexpr sys_clock::time_point
from_unix_millis(u64 millis) noexcept;
```

Behavior:
- Constructs a `time_point` whose `time_since_epoch()` equals the provided duration.
- Assumes UTC semantics.
- Does not perform timezone adjustments.

---

### Conversion to UNIX timestamps

| Function | Returns |
|----------|---------|
| `to_unix_seconds(tp)` | `u64` seconds since epoch |
| `to_unix_millis(tp)`  | `u64` milliseconds since epoch |

```cpp
[[nodiscard]] inline constexpr u64
to_unix_seconds(sys_clock::time_point tp) noexcept;

[[nodiscard]] inline constexpr u64
to_unix_millis(sys_clock::time_point tp) noexcept;
```

Behavior:
- Uses `std::chrono::duration_cast`.
- Truncates toward zero.
- Assumes timestamp is representable within `u64`.

---

### Current UNIX Time

| Function | Precision |
|----------|-----------|
| `unix_seconds_now()` | Seconds |
| `unix_millis_now()`  | Milliseconds |

```cpp
[[nodiscard]] inline u64
unix_seconds_now() noexcept;

[[nodiscard]] inline u64
unix_millis_now() noexcept;
```

Behavior:
- Obtains `system_clock::now()`.
- Converts using the corresponding `to_unix_*` function.
- Platform portable.

---

## stop_watch

Lightweight elapsed time measurement utility based on `high_resolution_clock`.

### Definition

```cpp
struct stop_watch
{
    hr_clock::time_point start_point{ hr_clock::now() };

    template<class Duration = std::chrono::milliseconds>
    [[nodiscard]] inline u64 elapsed() const noexcept;

    inline void reset() noexcept;
};
```

---

### Members

| Member | Description |
|--------|-------------|
| `start_point` | Captured start timestamp |
| `elapsed<Duration>()` | Returns elapsed time since start |
| `reset()` | Resets start timestamp |

---

### Behavior

- `elapsed<Duration>()` performs `duration_cast`.
- Default unit: milliseconds.
- Returns `u64`.
- No dynamic allocation.
- No synchronization guarantees.

---

## Design Characteristics

- C++23 compliant.
- No timezone abstraction (UTC only).
- No calendar formatting.
- Header-only.
- Fully portable across 32-bit and 64-bit systems.
- Compatible with binary parsing scenarios.

---

## Intended Usage

- Parsing file formats storing UNIX timestamps.
- Logging systems.
- Performance measurement.
- Binary analysis tooling.
- Systems-level utilities.

---

# Example Usage

---

## Constructing a Time Point

```cpp
stx::u64 ts = 1700000000;
auto tp = stx::from_unix_seconds(ts);
```

---

## Converting Back to UNIX Time

```cpp
auto now_tp = stx::sys_clock::now();

stx::u64 seconds = stx::to_unix_seconds(now_tp);
stx::u64 millis  = stx::to_unix_millis(now_tp);
```

---

## Getting Current UNIX Time

```cpp
stx::u64 s = stx::unix_seconds_now();
stx::u64 m = stx::unix_millis_now();
```

---

## Measuring Elapsed Time

```cpp
stx::stop_watch sw;

// some work here

stx::u64 elapsed_ms = sw.elapsed<>();
```

Custom duration:

```cpp
stx::u64 elapsed_ns =
    sw.elapsed<std::chrono::nanoseconds>();
```

Resetting:

```cpp
sw.reset();
```
