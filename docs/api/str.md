# String Literals (`str.hpp`)

Compile-time string literal transformations with natural decay.

## Header

```cpp
#include <lbyte/stx/str.hpp>   // basic_sl<CharT,N>, operator""_sl
```

## Overview

`basic_sl<CharT,N>` is a compile-time string builder in `lbyte::stx::details`.
It is obtained through the `operator""_sl` literal suffix.

```cpp
using namespace lbyte::stx::literals;

constexpr auto sl = "hello"_sl;
const char* s = sl;            // decay: operator const CharT* (lvalue only)
std::string_view sv = sl.sv(); // explicit: string_view (lvalue only)
auto cstr = sl.c_str();        // explicit: const char* (lvalue only)
auto str = sl.str();           // explicit: std::string (always safe)
```

## Literal Suffixes

| Suffix | `CharT` | Decay To |
|--------|---------|----------|
| `""_sl` | `char` | `const char*` |
| `u8""_sl` | `char8_t` | `const char8_t*` |
| `L""_sl` | `wchar_t` | `const wchar_t*` |
| `u""_sl` | `char16_t` | `const char16_t*` |
| `U""_sl` | `char32_t` | `const char32_t*` |

## Methods

| Member | Returns | Description |
|--------|---------|-------------|
| `operator const CharT*()` | `const CharT*` | Natural decay (like a raw literal); `const &` only — `&&` deleted |
| `sv()` | `std::basic_string_view<CharT>` | View into internal buffer; `const &` only — `&&` deleted |
| `c_str()` | `const CharT*` | Explicit C string pointer; `const &` only — `&&` deleted |
| `str()` | `std::basic_string<CharT>` | Owned copy — always safe on any ref-qualifier |
| `trim()` | `basic_sl<CharT, N>` | Remove leading + trailing `\n` only (internal newlines preserved) |
| `unindent()` | `basic_sl<CharT, N>` | Remove common leading whitespace based on first text line's indentation |

## Lifetime

All pointer/view accessors (`operator const CharT*`, `sv()`, `c_str()`) are
guarded with ref-qualifiers: they are **deleted on rvalue temporaries** to
prevent dangling at compile time. Store the result first:

```cpp
// ✅ valid: named lvalue
constexpr auto s = "\n  hello\n"_sl.trim().unindent();
const char* p = s;                  // safe
std::string_view sv = s.sv();       // safe
auto cstr = s.c_str();              // safe
std::string str = s.str();          // safe
```

```cpp
// ❌ invalid (compile error): temporary + guarded accessor
// const char* p = "\n  hello\n"_sl.trim().unindent();     // operator&& = delete
// auto sv = "\n  hello\n"_sl.trim().unindent().sv();      // sv() && = delete
// auto p  = "\n  hello\n"_sl.trim().unindent().c_str();   // c_str() && = delete
```

For one-shot usage, use `.str()` (always safe, returns owned copy):

```cpp
std::string out = "\n  hello\n"_sl.trim().unindent().str();   // ✅ safe
puts(out.c_str());
```

## trim()

Removes all leading and trailing newline characters (`\n`). Internal newlines
are preserved. Only `\n` is stripped — other whitespace characters (` `, `\t`,
`\r`) are left untouched. The result is null-terminated within the same-size
internal buffer; use `.sv()` to read the trimmed content.

```cpp
constexpr auto t1 = "\nhello\n"_sl.trim();       t1.sv()   // "hello"
constexpr auto t2 = "\n\nhello"_sl.trim();       t2.sv()   // "hello"
constexpr auto t3 = "hello\n\n"_sl.trim();       t3.sv()   // "hello"
constexpr auto t4 = "\nhello\nworld\n"_sl.trim(); t4.sv()  // "hello\nworld"
```

## unindent()

Walks every line and removes the first *N* whitespace characters (` ` and `\t`),
where *N* is the indentation of the **first non-empty line** — the first line
that contains at least one non-whitespace character. Lines consisting entirely
of whitespace are skipped when determining *N* but are preserved in the output.

Blank lines (containing only `\n`) keep their `\n` and do not reset the
indentation search.

```cpp
constexpr auto u1 = "  hello"_sl.unindent();             u1.sv()  // "hello"
constexpr auto u2 = "  hello\n  world"_sl.unindent();   u2.sv()  // "hello\nworld"
constexpr auto u3 = "  hello\n    world"_sl.unindent(); u3.sv()  // "hello\n  world"

// First non-empty line determines indent:
constexpr auto u4 = "\n  hello\n  world"_sl.unindent(); u4.sv()  // "\nhello\nworld"
```

## Chaining

```cpp
constexpr auto s = "\n  hello\n  world\n"_sl.trim().unindent();
// s.sv() → "hello\nworld"
```

## C/C++ Comparison

| Language | String Literal | Transform |
|----------|---------------|-----------|
| C | `"..."` | Manual loops |
| C++ | `"..."_sl` | `.trim().unindent()` compile-time |
| Python | `"""..."""` + `.strip()` + `inspect.cleandoc()` | Runtime |

## Module

```cpp
import lbyte.stx;   // all modules including str
```

The `_sl` literal is available through `import lbyte.stx.literals`.
