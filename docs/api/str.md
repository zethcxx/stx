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

const char* s = "hello"_sl;    // decay: operator const CharT*
auto sv = "hello"_sl.sv();     // explicit: string_view
auto str = "hello"_sl.str();   // explicit: std::string
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
| `operator const CharT*()` | `const CharT*` | Natural decay (like a raw literal) |
| `sv()` | `std::basic_string_view<CharT>` | View (never dangles if object alive) |
| `str()` | `std::basic_string<CharT>` | Owned copy |
| `trim()` | `basic_sl<CharT,N>` | Remove leading + trailing `\n` |
| `unindent()` | `basic_sl<CharT,N>` | Remove common leading whitespace based on first text line |

## Lifetime

`operator const CharT*()` returns a pointer to the internal buffer. If the
`basic_sl` object is a temporary, this pointer dangles. Keep the object alive:

```cpp
auto s = "  hello"_sl.trim().unindent();     // keep alive
const char* p = s;                            // safe
std::string_view sv = s.sv();                 // safe
std::string str = s.str();                    // safe
```

Or use `.sv()` / `.str()` in the same full-expression:

```cpp
puts("  hello"_sl.trim().unindent());          // safe: temp alive until ';'
auto sv = "  hello"_sl.trim().unindent().sv(); // safe: sv copied by value
// const char* p = "  hello"_sl.trim().unindent(); // DANGLES
```

## trim()

Removes all leading and trailing newline characters (`\n`).

```cpp
"\nhello\n"_sl.trim().sv()   // "hello"
"\n\nhello"_sl.trim().sv()   // "hello"
"hello\n\n"_sl.trim().sv()   // "hello"
"\nhello\nworld\n"_sl.trim().sv()  // "hello\nworld"
```

## unindent()

Detects the indentation of the first non-empty line (a line with at least one
non-whitespace character: spaces and tabs are whitespace, `\n` is line
separator). Removes that many leading whitespace characters from every line.

```cpp
"  hello"_sl.unindent().sv()            // "hello"
"  hello\n  world"_sl.unindent().sv()   // "hello\nworld"
"  hello\n    world"_sl.unindent().sv() // "hello\n  world"

// First non-empty line determines indent:
"\n  hello\n  world"_sl.unindent().sv() // "\nhello\nworld"
```

## Chaining

```cpp
auto s = "\n  hello\n  world\n"_sl.trim().unindent();
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
import lbyte.stx.str;   // basic_sl, but mainly for module internal use
import lbyte.stx;        // all modules including str
```

The `_sl` literal is available through `import lbyte.stx.literals`.
