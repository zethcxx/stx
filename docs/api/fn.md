# fn.hpp

## Overview

`fn.hpp` provides a lightweight, zero-overhead abstraction for invoking functions located at arbitrary memory addresses. It builds on `core.hpp`, leveraging:

- `address_like`
- `normalize_addr`
- Strong typing discipline for addresses

The primary goal is to safely reinterpret a normalized address as a function pointer with a specific signature and invoke it through a strongly-typed wrapper.

---

## Namespace: `stx`

---

## Template: `caller_t<Sig>`

Primary template declaration:

```cpp
template<class Sig>
struct caller_t;
```

Specialization for function types:

```cpp
template<class Ret, class... Args>
struct caller_t<Ret(Args...)>
{
    using fn_t = Ret (*)(Args...);
    fn_t fn;

    inline constexpr Ret operator()(Args... args) const noexcept
    {
        return fn(args...);
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept {
        return fn != nullptr;
    }
};
```

---

## Design Characteristics

| Property                  | Description |
|---------------------------|-------------|
| Zero abstraction overhead | Wraps raw function pointer |
| Strong signature binding  | Signature enforced at compile time |
| Explicit validity check   | `operator bool()` verifies non-null |
| `constexpr` friendly      | Fully usable in constant evaluation contexts (if address is valid) |
| `noexcept` call operator  | Assumes target function does not violate contract |

---

## Member: `fn`

| Member | Type                  | Description |
|--------|----------------------|-------------|
| `fn`   | `Ret (*)(Args...)`    | Raw function pointer |

No ownership semantics. No lifetime guarantees. No validation beyond null check.

---

## Invocation Operator

```cpp
inline constexpr Ret operator()(Args... args) const noexcept;
```

Directly forwards arguments to the stored function pointer.

Precondition:
- `fn != nullptr`
- The address is valid and correctly aligned for the specified signature.

Undefined behavior occurs if the signature does not match the actual function at the address.

---

## Conversion to bool

```cpp
explicit operator bool() const noexcept;
```

Returns `true` if `fn != nullptr`.

Example:

```cpp
stx::caller_t<int(int)> c{nullptr};

if (!c) {
    // safe null check
}
```

---

## Factory Function: `caller<Sig>`

```cpp
template<class Sig>
inline constexpr auto caller(address_like auto addr) noexcept;
```

Creates a `caller_t<Sig>` from any `address_like` value.

### Steps Performed

1. Normalize address to `uptr` via `normalize_addr`.
2. Reinterpret as `Ret (*)(Args...)`.
3. Store inside `caller_t<Sig>`.

Implementation:

```cpp
using fn_t = typename caller_t<Sig>::fn_t;
return caller_t<Sig>{
    reinterpret_cast<fn_t>(normalize_addr(addr))
};
```

---

## Accepted Address Types

Because it uses `address_like`, the following are valid inputs:

| Type              | Accepted |
|-------------------|----------|
| Raw pointer       | Yes      |
| `std::uintptr_t`  | Yes      |
| `std::intptr_t`   | Yes      |
| `stx::va_t`       | Yes      |
| `stx::rva_t`      | No       |
| `stx::offset_t`   | No       |

---

# Usage Examples

---

## 1. Calling a Known Function

```cpp
int add(int a, int b)
{
    return a + b;
}

auto c = stx::caller<int(int, int)>(&add);

int result = c(10, 20);  // 30
```

---

## 2. Using `va_t`

```cpp
stx::va_t address{ reinterpret_cast<stx::uptr>(&add) };

auto c = stx::caller<int(int, int)>(address);

int result = c(3, 4);
```

---

## 3. Using `std::uintptr_t`

```cpp
std::uintptr_t raw = reinterpret_cast<std::uintptr_t>(&add);

auto c = stx::caller<int(int, int)>(raw);

int result = c(5, 6);
```

---

## 4. Null Check

```cpp
auto c = stx::caller<int(int, int)>(std::uintptr_t{0});

if (!c)
{
    // function pointer is null
}
```

---

## 5. Calling an External Symbol by Address

Example scenario: resolved symbol from dynamic loader.

```cpp
stx::uptr symbol_addr = resolve_symbol("some_function");

auto fn = stx::caller<void(int)>(symbol_addr);

if (fn)
{
    fn(42);
}
```

---

## 6. Member Function Not Supported

Only free functions with C-style function pointers are supported.

Invalid:

```cpp
struct X {
    int method(int);
};

// auto invalid = stx::caller<int(int)>(&X::method); // ill-formed
```

Reason:
- Pointer-to-member function types are incompatible with `Ret (*)(Args...)`.

---

## Safety Considerations

| Risk                              | Explanation |
|-----------------------------------|-------------|
| Signature mismatch                | Leads to undefined behavior |
| Invalid memory address            | Undefined behavior |
| Calling convention mismatch       | Undefined behavior |
| Incorrect alignment               | Undefined behavior |
| Non-static member functions       | Not supported |

This utility performs no runtime validation.

---

## Intended Use Cases

- Manual symbol resolution
- Runtime patching
- Hooking systems
- Loader implementations
- Low-level tooling
- JIT integration layers

---

## Summary

`fn.hpp` provides:

- A strongly-typed wrapper over raw function addresses
- Signature-safe invocation
- Uniform address normalization via `core.hpp`
- Zero-cost abstraction

It assumes the caller guarantees that:

- The address is valid.
- The function signature matches exactly.
- The calling convention is correct.
- The lifetime of the function remains valid.

No additional safety layer is introduced beyond null detection.
