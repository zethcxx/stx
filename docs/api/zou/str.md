# String Literals (`str.hpp`)

Compile-time string literal transformations with `static` storage.

## Header

```cpp
#include <lbyte/zou/str.hpp>
```

## Overview

`lit::str<"str", flags...>` transforms a raw string literal at compile time and
exposes the result as `operator const CharT*()` and `str()`. The transformed
data lives in a `static constexpr` member — permanent storage duration (like a
string literal in `.rodata`). No temporary lifetime issues.

```cpp
using namespace lbyte::zou;

auto x = lit::str<"hello">;
const char* p = x;      // operator const CharT* — always valid, .rodata
std::string s = x.str(); // owned copy
```

## Flags

| Flag                | Effect                                                                 |
|---------------------|------------------------------------------------------------------------|
| *(none)*            | Raw string, no transformation                                          |
| `lit::strip`        | Remove first line if empty (whitespace-only) and last line if empty    |
| `lit::unindent`     | Strip common leading whitespace based on first text line's indentation |

## Usage

```cpp
using namespace lbyte::zou;

// Raw (no transform)
auto a = lit::str<"hello">;
a.str();                        // "hello"

// Strip surrounding empty lines
auto b = lit::str<"\nhello\n", lit::strip>;
std::string_view{b};            // "hello"

// Unindent
auto c = lit::str<"  hello\n  world", lit::unindent>;
std::string_view{c};            // "hello\nworld"

// Combine flags
auto d = lit::str<"\n  hello\n  world\n", lit::strip, lit::unindent>;
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
std::string_view{ lit::str<"\nhello\n",     lit::strip> }  // "hello"
std::string_view{ lit::str<"\n\n\nhello",   lit::strip> }  // "\n\nhello"
std::string_view{ lit::str<"hello\n\n\n",   lit::strip> }  // "hello\n\n"
std::string_view{ lit::str<"\nhello\nworld\n", lit::strip> } // "hello\nworld"

// Whitespace-only lines also qualify as "empty"
std::string_view{ lit::str<"  \nhello\n  ", lit::strip> } // "hello"
```

## unindent

Walks every line and removes the first *N* whitespace characters (` ` and `\t`),
where *N* is the indentation of the **first non-empty line** — the first line
that contains at least one non-whitespace character. Lines consisting entirely
of whitespace are skipped when determining *N* but are preserved in the output.
Blank lines (containing only `\n`) keep their `\n` and do not reset the search.

```cpp
std::string_view{ lit::str<"  hello",            lit::unindent> } // "hello"
std::string_view{ lit::str<"  hello\n  world",   lit::unindent> } // "hello\nworld"
std::string_view{ lit::str<"  hello\n    world", lit::unindent> } // "hello\n  world"

// First non-empty line determines indent:
std::string_view{ lit::str<"\n  hello\n  world", lit::unindent> } // "\nhello\nworld"
```

## Combined

```cpp
std::string_view{ lit::str<"\n  hello\n  world\n", lit::strip, lit::unindent> }
// "hello\nworld"
```

## `constexpr` context

```cpp
constexpr auto x = lit::str<"\n  hello\n  world\n", lit::unindent, lit::strip>;
static_assert( std::string_view{x} == "hello\nworld" );
```

## `lit::fstr<N>` — fixed string NTTP

`fstr<N>` is the structural type that wraps a string literal for use as a
non-type template parameter. It is the foundation of `lit::str` and can be
used directly in your own compile-time templates.

```cpp
using namespace lbyte::zou;

// Basic usage
constexpr lit::fstr s{ "hello" };
static_assert( s.size() == 5 );
static_assert( s[0] == 'h' );

// NTTP in your own templates
template<lit::fstr Str>
    requires (Str.size() > 0)
constexpr auto make_hex() noexcept { /* your decode logic */ }
```

## `lit::bytesof<Str, Order>` — compile-time byte pack

Packs a string literal into an unsigned integer of the smallest matching width, or a `std::string_view` for longer strings. The byte order is controllable via the second NTTP (default `endian::little`).

```cpp
using namespace lbyte::zou;
using namespace lbyte::stx::endian; // for order::little / order::big

// N == 1 → u8
static_assert( lit::bytesof<"\x01">                          == u8{0x01} );

// N == 2 → u16
static_assert( lit::bytesof<"\x01\x02">                      == u16{0x0201} );
static_assert( lit::bytesof<"\x01\x02", order::big>          == u16{0x0102} );

// N == 3..4 → u32
static_assert( lit::bytesof<"ABCD", order::big>              == u32{0x41424344} );

// N == 5..8 → u64
static_assert( lit::bytesof<"ABCDEFGH", order::big>          == u64{0x4142434445464748} );

// N > 8 → std::string_view (backed by .rodata)
auto sv = lit::bytesof<"hello world">;  // std::string_view
```

### NTTP parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `Str` | `fstr` | required | String literal to pack |
| `Order` | `endian::order` | `endian::little` | Byte order for packing |

### Return type by size

| `N = Str.size()` | Return type |
|------------------|-------------|
| 0 or 1 | `u8` |
| 2 | `u16` |
| 3–4 | `u32` |
| 5–8 | `u64` |
| >8 | `std::string_view` (eternal `.rodata`) |

## Variable template syntax

`lit::str` is a **variable template** — no `{}` or `()` needed:

```cpp
auto x = lit::str<"hello", lit::strip>;     // ← no braces
constexpr auto y = lit::str<"hello">;       // ← constexpr works
```

## C/C++ Comparison

| Language | String Literal                  | Transform                         |
|----------|---------------------------------|-----------------------------------|
| C        | `"..."`                         | Manual loops                      |
| C++ (zou)| `lit::str<"...", flags>`        | Compile-time, `.rodata`           |
| Python   | `"""..."""` + `.strip()` + `...`| Runtime                           |

## Module

```cpp
import lbyte.zou;   // includes lit::str
import lbyte.zou.str; // or just the str module
```
