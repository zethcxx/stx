# core.hpp

## Overview

`core.hpp` defines foundational types, concepts, and low-level utilities for the `stx` namespace under C++23. It centralizes:

- Fixed-width numeric aliases.
- Strongly-typed wrappers for address-related values.
- Core concepts for binary and address semantics.
- Address normalization utilities.
- Version metadata.

The design emphasizes type safety, ABI clarity, and compile-time validation using C++23 features (concepts, explicit object parameters, three-way comparison).

---

## Namespace: `stx`

### Versioning

| Symbol        | Type          | Description                          |
|--------------|--------------|--------------------------------------|
| `version_info` | struct        | Semantic version container           |
| `version`      | `constexpr`   | Current library version (`1.0.0`)    |

```cpp
struct version_info { int major, minor, patch; };
inline constexpr version_info version { 1, 0, 0 };
```

---

## Fundamental Type Aliases

Fixed-width and platform-consistent aliases.

### Unsigned Integers

| Alias | Underlying Type        |
|-------|------------------------|
| `u8`  | `std::uint8_t`         |
| `u16` | `std::uint16_t`        |
| `u32` | `std::uint32_t`        |
| `u64` | `std::uint64_t`        |

### Signed Integers

| Alias | Underlying Type       |
|-------|-----------------------|
| `i8`  | `std::int8_t`         |
| `i16` | `std::int16_t`        |
| `i32` | `std::int32_t`        |
| `i64` | `std::int64_t`        |

### Floating Point

| Alias | Underlying Type |
|-------|----------------|
| `f32` | `float`        |
| `f64` | `double`       |

### Size and Pointer Types

| Alias  | Underlying Type       | Purpose                          |
|--------|-----------------------|----------------------------------|
| `isize`| `std::ptrdiff_t`      | Signed size / difference type    |
| `usize`| `std::size_t`         | Unsigned size type               |
| `uptr` | `std::uintptr_t`      | Unsigned pointer-sized integer   |
| `iptr` | `std::intptr_t`       | Signed pointer-sized integer     |

---

## Strong Types

Strong types prevent implicit mixing of semantically distinct numeric domains while retaining zero-overhead abstraction.

### Declared Tags

```cpp
struct offset_tag {};
struct rva_tag    {};
struct va_tag     {};
```

### Public Aliases

| Alias       | Underlying Type | Semantic Meaning                |
|------------|-----------------|---------------------------------|
| `offset_t` | `usize`         | offset                         |
| `rva_t`    | `u32`           | Relative virtual address       |
| `va_t`     | `uptr`          | Absolute virtual address       |

These are implemented using:

```cpp
template<typename T, typename Tag>
class strong_type;
```

Each instantiation is type-distinct at compile time.

---

## Enum: `origin`

Alternative to `SEEK_SET`, `SEEK_CUR`, `SEEK_END`.

| Enumerator | Meaning             |
|------------|--------------------|
| `begin`    | From beginning     |
| `current`  | From current pos   |
| `end`      | From end           |

```cpp
enum class origin : u8
{
    begin,
    current,
    end,
};
```

---

## Concepts

### `address_like`

Defines types that can represent a memory address.

Satisfied if:

- Raw pointer
- `std::uintptr_t`
- `std::intptr_t`
- `va_t`

```cpp
template<typename Type>
concept address_like =
       std::is_pointer_v<Type>
    or std::same_as<std::remove_cv_t<Type>, std::uintptr_t>
    or std::same_as<std::remove_cv_t<Type>, std::intptr_t>
    or std::same_as<std::remove_cvref_t<Type>, va_t>;
```

Purpose:
- Constrains APIs requiring address normalization.
- Enforces explicit intent when mixing pointer and integer domains.

---

### `binary_readable`

Defines a type safe for raw binary deserialization.

Constraints:

| Requirement                     | Condition                                  |
|---------------------------------|--------------------------------------------|
| Trivially copyable              | `std::is_trivially_copyable_v<Type>`       |
| Standard layout                 | `std::is_standard_layout_v<Type>`          |
| Not empty                       | `!std::is_empty_v<Type>`                   |
| Not a pointer                   | `!std::is_pointer_v<Type>`                 |

```cpp
template<class Type>
concept binary_readable =
       std::is_trivially_copyable_v<Type>
    and std::is_standard_layout_v<Type>
    and not std::is_empty_v<Type>
    and not std::is_pointer_v<Type>;
```

Use case:
- Safe memory mapping.
- Structured binary parsing.
- `std::bit_cast` compatibility.

---

## Function: `normalize_addr`

Normalizes any `address_like` type to `uptr`.

```cpp
template<address_like Addr>
[[nodiscard]]
constexpr uptr normalize_addr(Addr base) noexcept;
```

### Behavior

| Input Type            | Normalization Strategy                         |
|----------------------|-----------------------------------------------|
| Pointer              | `reinterpret_cast<uptr>`                      |
| `va_t`               | Extract via `.get()` then cast                |
| Integer address type | `static_cast<uptr>`                           |

Purpose:
- Provides uniform address arithmetic domain.
- Eliminates ambiguity between pointer and integer forms.
- Centralizes address canonicalization logic.

---

## Implementation: `strong_type`

Located in `stx::details`.

### Template

```cpp
template<typename Type, typename Tag>
class strong_type;
```

### Design Properties

| Property                  | Description |
|---------------------------|-------------|
| Zero-overhead             | Stores only `Type` |
| Explicit construction     | Prevents implicit narrowing |
| Strong semantic tagging   | `Tag` enforces type distinction |
| Three-way comparison      | Defaulted `<=>` |
| Explicit object parameter | Uses C++23 `this` deducing syntax |

### Public Interface

#### Constructors

```cpp
constexpr strong_type() noexcept = default;
constexpr explicit strong_type(value_type) noexcept;

template<std::integral U>
constexpr explicit strong_type(U) noexcept;
```

#### Access

```cpp
template<typename Self>
constexpr auto&& get(this Self&&) noexcept;

template<typename Self>
constexpr explicit operator Type(this Self&&) noexcept;
```

#### Arithmetic

```cpp
friend constexpr strong_type operator+(strong_type, Type) noexcept;
friend constexpr strong_type operator-(strong_type, Type) noexcept;
friend constexpr Type operator-(strong_type, strong_type) noexcept;
```

#### Comparison

```cpp
friend constexpr auto operator<=>(const strong_type&, const strong_type&) = default;
```

### Invariants

- No implicit conversion to underlying type.
- No implicit cross-tag conversion.
- Arithmetic allowed only with underlying `Type`.
- Subtraction between two strong types returns raw difference.

---

## Design Characteristics

- C++23 explicit object parameter for symmetric value access.
- Strong compile-time constraints via concepts.
- ABI-stable fundamental types.
- Zero dynamic allocation.
- Header-only and constexpr-friendly.

---

## Intended Usage Domain

- Binary parsing frameworks.
- Executable format loaders.
- Memory inspection tools.
- File mapping systems.
- Low-level systems utilities.

The header provides a minimal, strongly-typed foundation for higher-level binary and memory abstractions.

# EXAMPLES
---

## Versioning

```cpp
static_assert(stx::version.major == 1);
static_assert(stx::version.minor == 0);
static_assert(stx::version.patch == 0);
```

---

## Fundamental Type Aliases

### Fixed-Width Types

```cpp
stx::u32  a = 10;
stx::i64  b = -42;
stx::f64  c = 3.1415926535;
stx::usize size = 1024;
stx::uptr  addr = 0x140000000;
```

---

## Strong Types

Strong types prevent accidental mixing of logically distinct address domains.

### Declared Aliases

| Alias       | Underlying | Meaning                   |
|------------|------------|----------------------------|
| `offset_t` | `usize`    | offset                     |
| `rva_t`    | `u32`      | Relative virtual address   |
| `va_t`     | `uptr`     | Absolute virtual address   |

---

### Construction

Explicit construction is required.

```cpp
stx::offset_t off  { 128 };
stx::rva_t    rva  { 0x2000 };
stx::va_t     base { 0x140000000 };
```

Integral construction is allowed but explicit:

```cpp
stx::offset_t off2 { 256u };
```

---

### Accessing Underlying Value

```cpp
stx::va_t v{ 0x1000 };
stx::uptr raw = v.get();
```

Explicit conversion:

```cpp
stx::uptr raw2 = static_cast<stx::uptr>(v);
```

---

### Arithmetic

Arithmetic is restricted to the underlying type.

```cpp
stx::offset_t off{100};
off = off + 20;        // ok
off = off - 10;        // ok

stx::offset_t a{200};
stx::offset_t b{150};
stx::usize diff = a - b;   // returns underlying difference
```

Cross-tag arithmetic is ill-formed:

```cpp
// stx::offset_t x{10};
// stx::rva_t y{20};
// auto invalid = x - y;  // compilation error
```

---

### Comparison

```cpp
stx::rva_t a{100};
stx::rva_t b{200};

if (a < b) {}
auto cmp = (a <=> b);
```

---

## Enum: `origin`

Replacement for traditional seek direction constants.

```cpp
stx::origin o = stx::origin::begin;
```

Example usage in a file abstraction:

```cpp
void seek(stx::offset_t offset, stx::origin where);
```

---

## Concept: `address_like`

Satisfied by:

- Raw pointers
- `std::uintptr_t`
- `std::intptr_t`
- `stx::va_t`

# Usage Examples

---

```cpp
template<stx::address_like Addr>
void print_addr(Addr a)
{
    stx::uptr normalized = stx::normalize_addr(a);
}
```

Valid calls:

```cpp
int value = 0;

print_addr(&value);
print_addr(static_cast<std::uintptr_t>(0x1000));
print_addr(stx::va_t{0x140000000});
```

Invalid:

```cpp
// print_addr(stx::rva_t{0x200});  // not address_like
```

---

## Function: `normalize_addr`

Signature:

```cpp
template<stx::address_like Addr>
constexpr stx::uptr normalize_addr(Addr base) noexcept;
```

Behavior:

| Input              | Result                         |
|-------------------|--------------------------------|
| `T*`               | Reinterpreted to `uptr`        |
| `va_t`             | Extracted underlying value     |
| Integer type       | Static cast to `uptr`          |

### Example

```cpp
int x = 0;

stx::uptr a = stx::normalize_addr(&x);
stx::uptr b = stx::normalize_addr(stx::va_t{0x1000});
stx::uptr c = stx::normalize_addr(std::uintptr_t{0x2000});
```

This centralizes canonical conversion to a uniform pointer-sized integer domain.

---

## Concept: `binary_readable`

Defines a type safe for raw binary deserialization.

### Requirements

| Constraint              | Enforced By                         |
|------------------------|--------------------------------------|
| Trivially copyable     | `std::is_trivially_copyable_v`       |
| Standard layout        | `std::is_standard_layout_v`          |
| Not empty              | `!std::is_empty_v`                   |
| Not pointer            | `!std::is_pointer_v`                 |

---

### Valid Type

```cpp
struct header
{
    stx::u32 magic;
    stx::u16 version;
    stx::u16 flags;
};

static_assert(stx::binary_readable<header>);
```

---

### Invalid Type

```cpp
struct invalid
{
    virtual ~invalid() = default;
};

static_assert(!stx::binary_readable<invalid>);
```

---

### Binary Read Example

```cpp
template<stx::binary_readable T>
T read_object(const void* buffer)
{
    return *static_cast<const T*>(buffer);
}
```

---

## Strong Type Internal Design (C++23 Features)

Uses:

- Explicit object parameter (`this Self&&`)
- Three-way comparison defaulting
- `std::integral` constrained constructor
- Zero-overhead storage

No implicit cross-domain conversion is permitted, ensuring semantic correctness in low-level systems code.

---

## Intended Usage

- PE/ELF/Mach-O parsing
- Memory inspection tooling
- Binary patching frameworks
- Memory-mapped file loaders
- Systems-level utilities

The header provides strict compile-time guarantees while remaining ABI-transparent and constexpr-compatible.
