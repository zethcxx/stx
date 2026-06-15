# Compile-Time Literals (`ct.hpp`)

Compile-time string literal transformations with `static` storage.

## Header

```cpp
#include <lbyte/zou/ct.hpp>
```

## Overview

`ct::str<"str", flags...>` transforms a raw string literal at compile time and
exposes the result as `operator const CharT*()`, `.data()`, and `.size()`. The transformed
data lives in a `static constexpr` member — permanent storage duration (like a
string literal in `.rodata`). No temporary lifetime issues.

```cpp
using namespace lbyte::stx;

auto x = ct::str<"hello">;
const char* p = x;         // operator const CharT* — always valid, .rodata
auto len = x.size();       // 5 — contiguous_buffer compatible
std::string s = x.str();   // owned copy

// works with mem::write (range overload via contiguous_buffer)
mem::write(buf, off_s{0}, x);
```

## Flags (`ct::fmt`)

```cpp
enum class fmt : unsigned char { none, strip, unindent };
```

| Flag                     | Effect                                                                 |
|--------------------------|------------------------------------------------------------------------|
|--------------------------|------------------------------------------------------------------------|
| *(none)*                 | Raw string, no transformation                                          |
| `ct::fmt::strip`        | Remove first line if empty (whitespace-only) and last line if empty    |
| `ct::fmt::unindent`     | Strip common leading whitespace based on first text line's indentation |

## Usage

```cpp
using namespace lbyte::stx;

// Raw (no transform)
auto a = ct::str<"hello">;
a.str();                        // "hello"

// Strip surrounding empty lines
auto b = ct::str<"\nhello\n", ct::fmt::strip>;
std::string_view{b};            // "hello"

// Unindent
auto c = ct::str<"  hello\n  world", ct::fmt::unindent>;
std::string_view{c};            // "hello\nworld"

// Combine flags
auto d = ct::str<"\n  hello\n  world\n", ct::fmt::strip, ct::fmt::unindent>;
std::string_view{d};            // "hello\nworld"

// Pointer to .rodata (eternal)
const char* p = d;
printf("%s\n", static_cast<const char*>(d));
```

## strip

Removes the **first line** if it is empty (whitespace-only) and the **last line**
if it is empty. Lines in the middle that happen to be empty are preserved.
Only the outermost lines are candidates — useful for `R"(...)"` raw literals or
heredoc-style strings where only the opening and closing delimiters should be
stripped.

```cpp
std::string_view{ ct::str<"\nhello\n",     ct::fmt::strip> }  // "hello"
std::string_view{ ct::str<"\n\n\nhello",   ct::fmt::strip> }  // "\n\nhello"
std::string_view{ ct::str<"hello\n\n\n",   ct::fmt::strip> }  // "hello\n\n"
std::string_view{ ct::str<"\nhello\nworld\n", ct::fmt::strip> } // "hello\nworld"

// Whitespace-only lines also qualify as "empty"
std::string_view{ ct::str<"  \nhello\n  ", ct::fmt::strip> } // "hello"
```

## unindent

Walks every line and removes the first *N* whitespace characters (` ` and `\t`),
where *N* is the indentation of the **first non-empty line** — the first line
that contains at least one non-whitespace character. Lines consisting entirely
of whitespace are skipped when determining *N* but are preserved in the output.
Blank lines (containing only `\n`) keep their `\n` and do not reset the search.

```cpp
std::string_view{ ct::str<"  hello",            ct::fmt::unindent> } // "hello"
std::string_view{ ct::str<"  hello\n  world",   ct::fmt::unindent> } // "hello\nworld"
std::string_view{ ct::str<"  hello\n    world", ct::fmt::unindent> } // "hello\n  world"

// First non-empty line determines indent:
std::string_view{ ct::str<"\n  hello\n  world", ct::fmt::unindent> } // "\nhello\nworld"
```

## Combined

```cpp
std::string_view{ ct::str<"\n  hello\n  world\n", ct::fmt::strip, ct::fmt::unindent> }
// "hello\nworld"
```

## `constexpr` context

```cpp
constexpr auto x = ct::str<"\n  hello\n  world\n", ct::fmt::unindent, ct::fmt::strip>;
static_assert( std::string_view{x} == "hello\nworld" );
```

## `ct::fstr<N>` — fixed string NTTP

`fstr<N>` is the structural type that wraps a string literal for use as a
non-type template parameter. It is the foundation of `ct::str` and can be
used directly in your own compile-time templates.

```cpp
using namespace lbyte::stx;

// Basic usage
constexpr ct::fstr s{ "hello" };
static_assert( s.size() == 5 );
static_assert( s[0] == 'h' );

// NTTP in your own templates
template<ct::fstr Str>
    requires (Str.size() > 0)
constexpr auto make_hex() noexcept { /* your decode logic */ }
```

## Variable template syntax

`ct::str` is a **variable template** — no `{}` or `()` needed:

```cpp
auto x = ct::str<"hello", ct::fmt::strip>;     // ← no braces
constexpr auto y = ct::str<"hello">;       // ← constexpr works
```

## C/C++ Comparison

| Language | String Literal                  | Transform                         |
|----------|---------------------------------|-----------------------------------|
| C        | `"..."`                         | Manual loops                      |
| C++ (stx)| `ct::str<"...", flags>`        | Compile-time, `.rodata`           |
| Python   | `"""..."""` + `.strip()` + `...`| Runtime                           |

## Module

```cpp
import lbyte.stx;   // includes ct::str
import lbyte.stx.str; // or just the str module
```

## `ct::istr<Str, Order>` — integral string

Packs ≤ 8 bytes into `u8`/`u16`/`u32`/`u64`. Compile error for larger strings.

```cpp
using namespace lbyte::stx;
static_assert( ct::istr<"\x01\x02">              == u16{0x0201} );
static_assert( ct::istr<"\x01\x02", endian::big> == u16{0x0102} );
```

## `ct::sstr<Str>` — static string (`std::string_view`, `.rodata`)

For strings > 8 bytes. Backed by a `static constexpr` array.

```cpp
auto sv = ct::sstr<"hello world">;  // string_view → .rodata
```

## `ct::vstr<Str>` — value string

For any N. N ≤ 8 → integer; N > 8 → `byte_block<N>` with `.data()` / `.size()`.

```cpp
auto cmd = ct::vstr<"calc.exe\0">;  // byte_block<9>
mem::write(buf, ct::vstr<"hello">); // uses value or range overload as appropriate
```
