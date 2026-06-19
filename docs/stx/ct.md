# Compile-Time Literals (`ct.hpp`)

Compile-time string literal transformations with `static` storage.

## Header

```cpp
#include <lbyte/stx/ct.hpp>
```

## Overview

`ct::str<"str", flags...>` transforms a raw string literal at compile time and
exposes the result as `operator const CharT*()`, `.data()`, and `.size()`. The transformed
data lives in a `static constexpr` member -- permanent storage duration (like a
string literal in `.rodata`). No temporary lifetime issues.

```cpp
using namespace lbyte::stx;

auto x = ct::str<"hello">;
const char* p = x;         // operator const CharT* -- always valid, .rodata
auto len = x.size();       // 5
std::string s = x.str();   // owned copy

// works with mem::write (range overload via contiguous_buffer)
mem::write(buf, off_s{0}, x);
```

## Flags (`ct::fmt`)

Each flag is a **type** inside `struct fmt` -- not an enum value. Parameterized
transforms take template arguments.

| Flag / Transform | Effect |
|---|---|
| *(none)* | Raw string, no transformation |
| `fmt::strip` | Remove first line if empty (whitespace-only) and last line if empty |
| `fmt::unindent` | Strip common leading whitespace based on first text line's indentation |
| `fmt::trim_left` | Remove leading whitespace (spaces/tabs) |
| `fmt::trim_right` | Remove trailing whitespace (spaces/tabs) |
| `fmt::trim_trailing_lines` | Trim trailing whitespace on each line |
| `fmt::collapse_blank_lines` | Collapse consecutive blank lines into one |
| `fmt::replace_all\<"from", "to"\>` | Replace all occurrences of `from` with `to` |
| `fmt::strip_line_comments\<"//"\>` | Remove line comments starting with a marker |
| `fmt::chain\<Fs...\>` | Apply multiple transforms in order |
| `fmt::trim_block` | Preset: `chain\<strip, unindent\>` |

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
auto d = ct::str<"\n  hello\n  world\n", ct::fmt::trim_block>;
std::string_view{d};            // "hello\nworld"

// Pointer to .rodata (eternal)
const char* p = d;
printf("%s\n", static_cast<const char*>(d));
```

## strip

Removes the **first line** if it is empty (whitespace-only) and the **last line**
if it is empty. Lines in the middle that happen to be empty are preserved.
Only the outermost lines are candidates -- useful for `R"(...)"` raw literals or
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
where *N* is the indentation of the **first non-empty line** -- the first line
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

## trim_block

Convenience alias for `fmt::chain<fmt::strip, fmt::unindent>`. The idiomatic
way to clean up block-heredoc-style strings:

```cpp
std::string_view{ ct::str<"\n  hello\n  world\n", ct::fmt::trim_block> }
// "hello\nworld"
```

## replace_all

Replaces all occurrences of `From` with `To`. Requires `To.size() <= From.size()`
to guarantee in-place safety at compile time.

```cpp
std::string_view{ ct::str<"a-b-c", ct::fmt::replace_all<"-", "_">> }
// "a_b_c"
```

## strip_line_comments

Removes single-line comments starting with a given marker (e.g. `//`, `#`, `;`).

```cpp
std::string_view{ ct::str<"a\n// b\nc", ct::fmt::strip_line_comments<"//">> }
// "a\nc"
```

## chain

Applies a sequence of transforms in declaration order (left to right).

```cpp
std::string_view{ ct::str<"\n  a\n  b\n",
    ct::fmt::chain<ct::fmt::strip, ct::fmt::unindent, ct::fmt::replace_all<"a", "z">>> }
// "z\nb"
```

## `constexpr` context

```cpp
constexpr auto x = ct::str<"\n  hello\n  world\n", ct::fmt::trim_block>;
static_assert( std::string_view{x} == "hello\nworld" );
```

## `str_type::apply<MoreFlags...>()` -- chaining transforms on computed values

Apply additional transforms to an already-transformed `str_type`. Returns a new
`str_type` with the combined flag list.

```cpp
constexpr auto x = ct::str<"-hello-", ct::fmt::trim_left>;
// x == "hello-"
constexpr auto y = decltype(x)::apply<ct::fmt::trim_right>();
// y == "hello"
```

## `ct::fixed_string<N>` -- fixed string NTTP

`fixed_string<N>` is the structural type that wraps a string literal for use as a
non-type template parameter. It is the foundation of `ct::str` and can be
used directly in your own compile-time templates.

```cpp
using namespace lbyte::stx;

// Basic usage
constexpr ct::fixed_string s{ "hello" };
static_assert( s.size() == 5 );
static_assert( s[0] == 'h' );

// NTTP in your own templates
template<ct::fixed_string Str>
    requires (Str.size() > 0)
constexpr auto make_hex() noexcept { /* your decode logic */ }
```

## Variable template syntax

`ct::str` is a **variable template** -- no `{}` or `()` needed:

```cpp
auto x = ct::str<"hello", ct::fmt::strip>;     // no braces
constexpr auto y = ct::str<"hello">;       // constexpr works
```

## Format strings with `ct::args<Vs...>`

`ct::args<Vs...>` expands `{}` placeholders in a string literal at compile time.
Placeholders can include format specs like `{:x}`, `{:>8}`, `{:*^10}`.

```cpp
using namespace lbyte::stx;

// Basic placeholder
auto a = ct::str<"val={}", ct::args<42>>;
std::string_view{a};            // "val=42"

// Hex, width, alignment, fill
auto b = ct::str<"0x{:X}", ct::args<255>>;
std::string_view{b};            // "0xFF"

auto c = ct::str<"[{:>8}]", ct::args<42>>;
std::string_view{c};            // "[      42]"

auto d = ct::str<"[{:*^10}]", ct::args<7>>;
std::string_view{d};            // "[****7*****]"
```

Supported format specifiers:
- `d` -- decimal (default)
- `x` / `X` -- lowercase / uppercase hex
- `o` -- octal
- `b` / `B` -- binary
- `c` -- char
- `>` / `<` / `^` -- right, left, center alignment
- Fill character before alignment (e.g. `*^`)
- Width (e.g. `>8`)

## Custom type formatting via `formatter<T>`

Specialize `ct::formatter<T>` to make custom types work with `args<...>`:

```cpp
template<>
struct ct::formatter<MyPoint> {
    static consteval size_t expanded_size(const ct::details::fmt_spec&) noexcept {
        return 16; // max representation size
    }
    static consteval void write_to(char* buf, MyPoint p, const ct::details::fmt_spec&) noexcept {
        // write "x,y" into buf
    }
};
```

## C/C++ Comparison

| Language | String Literal | Transform |
|----------|---------------------------------|-----------------------------------|
| C | `"..."` | Manual loops |
| C++ (stx)| `ct::str<"...", flags>` / `ct::str<"...", ct::args<...>>` | Compile-time, `.rodata` |
| Python | `"""..."""` + `.strip()` + `...`| Runtime |

## Module

```cpp
import lbyte.stx;   // includes ct::str
import lbyte.stx.ct; // or just the ct module
```

## `ct::istr<Str, T?, Order?>` -- integral string

Packs <= 8 bytes into `u8`/`u16`/`u32`/`u64`. Parameters are positional: string
(required), type (optional, defaults to auto-deduced), endian (optional, defaults
to `ct::endian::little`).

```cpp
using namespace lbyte::stx;
static_assert( ct::istr<"\x01\x02">                       == u16{0x0201}      );
static_assert( ct::istr<"\x01\x02", ct::endian::big>      == u16{0x0102}      );
static_assert( ct::istr<"MZ", u64>                        == u64{0x5A4D}      );
static_assert( ct::istr<"PE", u32>                        == u32{0x00004550}  );
static_assert( ct::istr<"PE", u32, ct::endian::big>       == u32{0x50450000}  );
```

Note: `ct::endian::big` and `ct::endian::little` are type tags (not enum values).

## `ct::byte_block<N>` -- raw byte array

A fixed-size byte array with `.data()` and `.size()`. Useful for binary I/O.

```cpp
ct::byte_block<4> blk{};
```

## `ct::vstr<Str>` / `ct::vstr<Str, N>` -- value string (`ct::byte_block<N>`)

Packs a string into a `ct::byte_block<N>`. If `N > Str.size()`, the extra bytes
are zero-padded. If `N == Str.size()` (default), exact fit.

```cpp
auto sig = ct::vstr<"PE", 4>;   // byte_block<4>{'P','E',0,0}
auto cmd = ct::vstr<"cmd.exe">; // byte_block<7>{'c','m','d','.','e','x','e'}
```

## `ct::re<Pattern>` -- compile-time regex (via CTRE)

Available when CTRE is detectable (`__has_include(<ctre.hpp>)`). Include `<ctre.hpp>`
before or after `<lbyte/stx/ct.hpp>` -- `ct::re<...>` is defined inside `ct.hpp`
when CTRE is present. Adds two transforms compatible with `ct::str` and `ct::fmt::chain`:

| Transform | Effect |
|---|---|
| `re<Pattern>::replace<Replacement>` | Replace all matches of `Pattern` with `Replacement` |
| `re<Pattern>::remove` | Remove all matches of `Pattern` (shorthand for `replace<"">`) |

```cpp
// CTRE must be in your include path. Include in any order:
#include <ctre.hpp>
#include <lbyte/stx/ct.hpp>

using namespace lbyte::stx;

// Collapse multiple newlines to semicolon
auto a = ct::str<"a\n\n\nb", ct::re<"\n+">::replace<";">>;
std::string_view{a};            // "a;b"

// Collapse horizontal whitespace
auto b = ct::str<"a    b   c", ct::re<"\\s+">::replace<" ">>;
std::string_view{b};            // "a b c"

// Trim spaces around = (minify config)
auto c = ct::str<"e asm.nbytes      = 16", ct::re<"\\s*=\\s*">::replace<"=">>;
std::string_view{c};            // "e asm.nbytes=16"

// Remove matches entirely
auto d = ct::str<"a--b--c", ct::re<"--">::remove>;
std::string_view{d};            // "abc"

// Chained with fmt transforms
auto e = ct::str<"\n  hello\n  world\n", ct::fmt::chain<
    ct::fmt::trim_block,
    ct::re<"\n+">::replace<";">
>>;
std::string_view{e};            // "hello;world"
```

Notes:
- `Pattern` uses PCRE syntax (via CTRE) -- supports `\s`, `\d`, `\w`, `+`, `*`, etc.
- Unicode properties like `\p{Letter}`, `\p{Number}`, `\p{Uppercase}` are supported
  (requires `<ctre-unicode.hpp>`, auto-detected via `__has_include`).
- Patterns with zero-length matches (e.g. `\s*`) advance by one character to prevent infinite loops.
- Compatible with `ct::fmt::chain`, `ct::str`, and `str_type::apply()`.
```
