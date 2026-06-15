# Compile-Time Literals (`ct.hpp`)

Compile-time string literal transformations with `static` storage.

## Header

```cpp
#include <lbyte/zou/ct.hpp>
```

## Overview

`ct::str<"str", flags...>` transforms a raw string literal at compile time and
exposes the result as `operator const CharT*()` and `str()`. The transformed
data lives in a `static constexpr` member — permanent storage duration (like a
string literal in `.rodata`). No temporary lifetime issues.

```cpp
using namespace lbyte::zou;

auto x = ct::str<"hello">;
const char* p = x;      // operator const CharT* — always valid, .rodata
std::string s = x.str(); // owned copy
```

## Flags (`ct::fmt`)

| Flag                     | Effect                                                                 |
|--------------------------|------------------------------------------------------------------------|
| *(none)*                 | Raw string, no transformation                                          |
| `ct::fmt::strip`        | Remove first line if empty (whitespace-only) and last line if empty    |
| `ct::fmt::unindent`     | Strip common leading whitespace based on first text line's indentation |

## Usage

```cpp
using namespace lbyte::zou;

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
using namespace lbyte::zou;

// Basic usage
constexpr ct::fstr s{ "hello" };
static_assert( s.size() == 5 );
static_assert( s[0] == 'h' );

// NTTP in your own templates
template<ct::fstr Str>
    requires (Str.size() > 0)
constexpr auto make_hex() noexcept { /* your decode logic */ }
```

## `ct::istr<Str, Order>` — integral string (native integer)

Packs a string literal (≤ 8 bytes) into the smallest native unsigned integer. Compile error for strings > 8 bytes — use `sstr` or `vstr`.

```cpp
using namespace lbyte::zou;
using namespace lbyte::stx::endian; // for order::little / order::big

// N == 1 → u8
static_assert( ct::istr<"\x01">                          == u8{0x01} );

// N == 2 → u16
static_assert( ct::istr<"\x01\x02">                      == u16{0x0201} );
static_assert( ct::istr<"\x01\x02", endian::big>         == u16{0x0102} );

// N == 3..4 → u32
static_assert( ct::istr<"ABCD", endian::big>             == u32{0x41424344} );

// N == 5..8 → u64
static_assert( ct::istr<"ABCDEFGH", endian::big>         == u64{0x4142434445464748} );

// N > 8 → compile error
// ct::istr<"hello world">;  // error!

// With ct::endian:
using namespace ct::endian;
static_assert( ct::istr<"\x01\x02", big> == u16{0x0102} );
```

## `ct::sstr<Str>` — static string (backed by `.rodata`)

Returns a `std::string_view` backed by a `static constexpr` array (eternal storage like `.rodata`). Only for strings > 8 bytes; compile error otherwise.

```cpp
auto sv = ct::sstr<"hello world">;  // std::string_view → .rodata
const char* p = sv.data();          // always valid, no dangling
```

## `ct::vstr<Str>` — value string (by-value copy)

Returns a trivially-copyable value. For N ≤ 8: native integer (`u8`/`u16`/`u32`/`u64`). For N > 8: `byte_block<N>` — a simple struct with `.data()` and `.size()`.

```cpp
// N ≤ 8 → integer
auto v1 = ct::vstr<"abc">;   // u32

// N > 8 → byte_block<N>
volatile cmd = ct::vstr<"calc.exe\0">;
auto p = cmd.data();  // const u8*
auto n = cmd.size();  // 9

// mem::write uses the range overload (contiguous_buffer concept)
mem::write(buffer, ct::vstr<"hello world">);
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
| C++ (zou)| `ct::str<"...", flags>`        | Compile-time, `.rodata`           |
| Python   | `"""..."""` + `.strip()` + `...`| Runtime                           |

## Module

```cpp
import lbyte.zou;   // includes ct::str
import lbyte.zou.ct; // or just the ct module
```
