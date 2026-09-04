# Compile-Time Literals (`ct.hpp`)

Compile-time string literal transformations with `static` storage.

## Header

```cpp
#include <lbyte/stx/ct.hpp>
```

## Overview

`ct::str<"str", CharT = char, flags...>` transforms a raw string literal at
compile time and exposes the result as `operator const CharT*()`, `.data()`, and
`.size()`. The second template parameter lets you choose the character type
(default `char`). The transformed data lives in a `static constexpr` member --
permanent storage duration (like a string literal in `.rodata`). No temporary
lifetime issues.

Transforms produce an **exact-size** array: the content plus a trailing `\0`,
with no padding. `x.size()` is the content length, and array introspection
(`decltype(x)::value.size()`) reflects the exact content too. Use
`fmt::fixed<Size>` to force a fixed-size buffer or `fmt::pad_end<N>` to grow
the buffer with trailing zero bytes. For multi-line text, `ct::eraw<R"(...)"...>`
(or `fmt::unescape`) reinterprets a raw string as a normal literal so escapes
like `\n` and `\033` work inside it.

```cpp
using namespace lbyte::stx;

auto x = ct::str<"hello">;
const char* p = x;              // operator const char* -- always valid, .rodata
auto len = x.size();            // 5
std::string s = x;              // implicit operator std::string
auto [cstr, n] = ct::str<"42">; // structured binding: const char* + size_t
std::string v = std::format("[{}]", x);  // std::formatter support

// works with mem::write (range overload via contiguous_buffer)
mem::write(buf, off_s{0}, x);
```

## Flags (`ct::fmt`)

Each flag is a **type** inside `struct fmt` -- not an enum value. Parameterized
transforms take template arguments.

| Flag / Transform                   | Effect                                                                  |
| ---------------------------------- | ----------------------------------------------------------------------- |
| *(none)*                           | Raw string, no transformation                                           |
| `fmt::strip`                       | Remove first line if empty (whitespace-only) and last line if empty     |
| `fmt::unindent`                    | Strip common leading whitespace based on first text line's indentation  |
| `fmt::trim_left`                   | Per-line: remove leading whitespace (spaces/tabs) from each line        |
| `fmt::trim_right`                  | Per-line: remove trailing whitespace (spaces/tabs) from each line       |
| `fmt::trim_trailing_lines`         | Trim trailing whitespace on each line                                   |
| `fmt::collapse_blank_lines`        | Collapse consecutive blank lines into one                               |
| `fmt::remove_blank_lines`          | Remove entirely blank (or whitespace-only) lines                        |
| `fmt::trim_each_line`              | Trim leading/trailing whitespace on each line                           |
| `fmt::collapse_whitespace`         | Collapse horizontal whitespace (spaces/tabs) to single space            |
| `fmt::replace_all\<"from", "to"\>` | Replace all occurrences of `from` with `to` (output may grow or shrink) |
| `fmt::strip_line_comments\<"//"\>` | Remove line comments starting with a marker                             |
| `fmt::fixed\<Size\>`               | Force the result to exactly `Size` bytes, null-terminated               |
| `fmt::pad_end\<N\>`                | Append `N` zero bytes after the `\0`, grow only (never truncates)       |
| `fmt::unescape`                    | Reinterpret a raw string as a normal literal (interpret escapes)        |
| `fmt::chain\<Fs...\>`              | Apply multiple transforms in order                                      |
| `fmt::trim_block`                  | Preset: `chain\<strip, unindent\>`                                      |

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

## remove_blank_lines

Removes any line that is empty or contains only whitespace (spaces/tabs).

```cpp
std::string_view{ ct::str<"a\n\n\nb",           ct::fmt::remove_blank_lines> } // "a\nb"
std::string_view{ ct::str<"a\n  \nb",           ct::fmt::remove_blank_lines> } // "a\nb"
std::string_view{ ct::str<"\n\na",              ct::fmt::remove_blank_lines> } // "a"
std::string_view{ ct::str<"hello",              ct::fmt::remove_blank_lines> } // "hello"
```

## trim_each_line

Trims leading and trailing whitespace from each line independently.

```cpp
std::string_view{ ct::str<"  hello\n  world",  ct::fmt::trim_each_line> } // "hello\nworld"
std::string_view{ ct::str<"hello   \nworld ",  ct::fmt::trim_each_line> } // "hello\nworld"
std::string_view{ ct::str<"  hello world  ",   ct::fmt::trim_each_line> } // "hello world"
```

## collapse_whitespace

Collapses sequences of horizontal whitespace (spaces and tabs) into a single space.

```cpp
std::string_view{ ct::str<"a    b   c",        ct::fmt::collapse_whitespace> } // "a b c"
std::string_view{ ct::str<"a\t\tb  c",         ct::fmt::collapse_whitespace> } // "a b c"
```

## replace_all

Replaces all occurrences of `From` with `To`. The output may grow or shrink, so
`To` is not limited to `From`'s size. `From` must be non-empty.

```cpp
std::string_view{ ct::str<"a-b-c", ct::fmt::replace_all<"-", "_">> }
// "a_b_c"

// Growing replacement
std::string_view{ ct::str<"a.b", ct::fmt::replace_all<".", " ==> ">> }
// "a ==> b"
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
std::string_view{ ct::str<"\n  a\n\n  b\n",
    ct::fmt::chain<ct::fmt::trim_block, ct::fmt::remove_blank_lines>> }
// "a\nb"

// Full minify pipeline
std::string_view{ ct::str<"\n  e asm.nbytes      = 16\n  e scr.color       = 1\n",
    ct::fmt::chain<
        ct::fmt::trim_block,
        ct::fmt::trim_each_line,
        ct::fmt::collapse_whitespace,
        ct::fmt::replace_all<" = ", "=">,
        ct::fmt::replace_all<"\n", ";">
    >> }
// "e asm.nbytes=16;e scr.color=1"
```

## fixed

Forces the result to a buffer of exactly `Size` bytes (including the trailing
`\0`). If the content is shorter it is null-padded; if it is longer it is
truncated to `Size - 1` characters. Use it last to fix the final layout.

```cpp
std::string_view{ ct::str<"hello", ct::fmt::fixed<8>> }
// "hello"   (value is a std::array<char, 8>, null-padded)

decltype(ct::str<"hello world", ct::fmt::fixed<8>>)::value.size()
// 8, content truncated to "hello w"

std::string_view{ ct::str<"  hello  \n", ct::fmt::trim_block, ct::fmt::fixed<12>> }
// "hello  "
```

## pad_end

Appends `N` zero bytes after the trailing `\0`, growing the buffer. Unlike
`fixed`, it **never truncates**: the content is always kept whole, so there is
no size to count. The extra zeros live in the backing array (`value`) but stay
invisible to `size()`, `string_view`, `format` and `operator const char*`
(which still stop at the first `\0`). `N` defaults to `1`.

```cpp
decltype(ct::str<"hello", ct::fmt::pad_end<2>>)::value.size()  // 8
// value bytes: h e l l o \0 \0 \0

std::string_view{ ct::str<"hello", ct::fmt::pad_end<2>> }      // "hello"
decltype(ct::str<"hello", ct::fmt::pad_end<2>>)::size()        // 5

ct::str<"...", ct::fmt::pad_end>      // N = 1
```

Typical use: a long string that must sit in a fixed-size field of a binary
layout, where you want a zeroed region after the text without having to count
the exact length (`fixed` would truncate if you undershoot).

## unescape

Reinterprets a **raw string** the way a normal C++ string literal would be
parsed: escape sequences become the actual characters. Normal literals do not
need this (the compiler already interprets `\n`); the point is that raw
literals cannot be multi-line without escapes, so writing

```cpp
"line1\n"
"line2"
// or
"line1\n\
line2"
```

is replaced by the comfortable form

```cpp
ct::str<R"(
line1
line2
)", ct::fmt::unescape, ct::fmt::strip>
// "line1\nline2"
```

Recognized sequences:

| Sequence      | Meaning                                             |
| ------------- | --------------------------------------------------- |
| `\n \t \r`    | newline, tab, carriage return                       |
| `\a \b \f \v` | bell, backspace, form feed, vertical tab            |
| `\' \" \? \\` | the literal character                               |
| `\NNN`        | octal (1-3 digits); `\0` ends the content           |
| `\xNN`        | hex (1+ digits), low byte kept                      |
| `\` + newline | line continuation: both removed (also handles CRLF) |

Anything else -- `\q`, `\u`/`\U` unicode names, a lone trailing `\` -- is kept
literally. ANSI codes need no special support: `\033[31m` becomes a single ESC
byte (`\033` octal) followed by the literal `[31m`.

```cpp
ct::str<R"(say \"hi\" \\ bye)", ct::fmt::unescape>  // say "hi" \ bye
ct::str<R"(\101\x41)",          ct::fmt::unescape>  // "AA"
ct::str<R"(\033[31m)",          ct::fmt::unescape>  // ESC + "[31m"
ct::str<R"(a\
b)",                            ct::fmt::unescape>  // "ab"
```

## eraw

Shorthand for `str<..., fmt::unescape, ...>`: reinterprets the raw literal and
then applies any remaining flags.

```cpp
ct::eraw<R"(line1\n\tline2)">           // "line1\n\tline2"
ct::eraw<R"(  hi\n  )", ct::fmt::trim_block>  // "hi"
```

Note: `unescape`/`eraw` do not combine with `ct::args` (the args path ignores
flags). For a one-off ANSI or escape string you can also just use a normal
literal, which the compiler already decodes: `ct::str<"\033[31m">`.

## Conversions

`str_type` converts directly to the common string views and to an owned copy,
with no lifetime concerns (data lives in `.rodata`):

```cpp
auto x = ct::str<"hello">;

const char* p = x;                 // operator const char*  -- null-terminated
std::string_view sv = x;           // operator std::string_view -- content only
std::string s = x;                 // operator std::string  -- owned copy
auto [cstr, len] = x;              // structured binding: const char* + size_t
                                   // cstr == x.data(), len == x.size()
std::string same = x.str();        // explicit owned copy
```

`std::string s = x` and `x.str()` are equivalent; both copy exactly the content
length (`x.size()`), not the backing array.

### `std::format` / `std::println`

`str_type` has a `std::formatter` specialization (whenever `<format>` is
available) that delegates to `std::formatter<std::string_view>`, so the full
string mini-language works: fill, alignment, width, precision.

```cpp
std::format("{}", ct::str<"hello">);        // "hello"
std::format("[{:>10}]", ct::str<"hello">);  // "[     hello]"
std::println("{}", ct::str<"  hi  ", ct::fmt::trim_block>);
```

> **C++20 modules:** a `std::formatter` specialization cannot be exported from a
> module (it lives in `namespace std`), so it is *not* visible through
> `import lbyte.stx.ct;` alone — `std::format`/`std::println` on `str_type` would
> report "formatter must be specialized". When using modules, `#include
> <lbyte/stx/ct.hpp>` in the importing translation unit (global module
> fragment). This is ODR-safe: the module compiles the same header, so `str_type`
> is the same entity in both.

## `constexpr` context

```cpp
constexpr auto x = ct::str<"\n  hello\n  world\n", ct::fmt::trim_block>;
static_assert( std::string_view{x} == "hello\nworld" );
```

## `str_type::apply<MoreFlags...>()` -- chaining transforms on computed values

Apply additional transforms to an already-transformed `str_type`. Returns a new
`str_type` holding the fully transformed result; transforms are applied exactly
once (no re-running of the original flags).

```cpp
constexpr auto x = ct::str<"  hello  ", ct::fmt::trim_left>;
// x == "hello  "
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

## Typed strings with custom `CharT`

The second template parameter selects the output character type. This is useful
for interop with non-`char` string types like `xmlChar`, `wchar_t`, etc.

```cpp
using namespace lbyte::stx;

// Default: char
auto a = ct::str<"hello">;
const char* p = a;

// Custom CharT
auto b = ct::str<"hello", wchar_t>;
const wchar_t* q = b;
```

With flags and format args:

```cpp
auto s = ct::str<"value={}", unsigned char, ct::args<42>>;
// s.data() -> const unsigned char*
```

`str_type::apply()` preserves the `CharT`:

```cpp
using T = decltype(ct::str<"-hello-", unsigned char, ct::fmt::trim_left>);
auto y = T::apply<ct::fmt::trim_right>();
// y is ct::str_type<"-hello-", unsigned char, ct::fmt::trim_left, ct::fmt::trim_right>
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

| Language  | String Literal                                            | Transform               |
| --------- | --------------------------------------------------------- | ----------------------- |
| C         | `"..."`                                                   | Manual loops            |
| C++ (stx) | `ct::str<"...", flags>` / `ct::str<"...", ct::args<...>>` | Compile-time, `.rodata` |
| Python    | `"""..."""` + `.strip()` + `...`                          | Runtime                 |

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
static_assert( ct::istr<"AB", u64>                        == u64{0x4241}      );
static_assert( ct::istr<"AB", u32>                        == u32{0x00004241}  );
static_assert( ct::istr<"AB", u32, ct::endian::big>       == u32{0x41420000}  );
```

Note: `ct::endian::big` and `ct::endian::little` are type tags (not enum values).

## `ct::byte_block<N>` -- raw byte array

Alias for `std::array<u8, N>`. Provides `.data()`, `.size()`, `operator[]`,
iteration, comparison, and all `std::array` operations. Useful for binary I/O.

```cpp
ct::byte_block<4> blk{};
auto p = blk.data();   // u8*
auto n = blk.size();   // 4
blk[0] = 0x50;         // direct indexing
```

## `ct::repeat<V, Reps>` -- repeat pattern

Repeats a pattern `V` (scalar or array-like with `value_type`/`tuple_size`) `Reps` times into a `std::array`.

```cpp
constexpr auto r1 = ct::repeat<u8{0xAB}, 4>;           // array<u8, 4>{0xAB,...}
constexpr auto r2 = ct::repeat<std::array{1,2,3}, 3>;  // array<int, 9>{1,2,3,1,2,3,...}
```

## `ct::vstr<Str>` / `ct::vstr<Str, N>` -- value string (`ct::byte_block<N>`)

Packs a string into a `ct::byte_block<N>`. If `N > Str.size()`, the extra bytes
are zero-padded. If `N == Str.size()` (default), exact fit.

```cpp
auto sig = ct::vstr<"AB", 4>;   // byte_block<4>{'A','B',0,0}
auto cmd = ct::vstr<"hello">;   // byte_block<5>{'h','e','l','l','o'}
```

## `ct::vstr_of<Str, Type>` -- typed value string

Packs a string into a fixed container of your choice (`std::array`,
`ct::byte_block`, ...). The element type and size come from `Type` (must expose
`value_type` + `std::tuple_size`). If the string is shorter than the container
it is zero-padded; if longer, it is truncated.

```cpp
using magic_type = std::array<char, 8>;
constexpr auto magic = ct::vstr_of<"ABCDEFGH", magic_type>;       // {'A','B','C','D','E','F','G','H'}
constexpr auto sig   = ct::vstr_of<"AB", std::array<char, 4>>;    // {'A','B',0,0}
constexpr auto blk   = ct::vstr_of<"AB", ct::byte_block<4>>;      // {0x41,0x42,0,0}
```

## `ct::re<Pattern>` -- compile-time regex transforms (optional)

When [CTRE](https://github.com/hanickadot/compile-time-regular-expressions) is
available (`__has_include(<ctre.hpp>)`), `ct::re` provides compile-time regex
replace and remove operations. They integrate with `ct::str` just like `fmt`
transforms (including within `fmt::chain`).

### `ct::re<Pattern>::replace<Replacement>`

```cpp
using namespace lbyte::stx;

// Basic replacement
auto a = ct::str<"a--b--c", ct::re<R"(--)">::replace<".">>;
std::string_view{a};            // "a.b.c"

// Regex pattern (collapse whitespace)
auto b = ct::str<"a   b  c", ct::re<R"(\s+)">::replace<" ">>;
std::string_view{b};            // "a b c"

// Replace digits with marker
auto c = ct::str<"abc123def456", ct::re<R"(\d+)">::replace<"#">>;
std::string_view{c};            // "abc#def#"
```

### `ct::re<Pattern>::remove`

Removes all matches of the pattern entirely.

```cpp
auto x = ct::str<"foo_bar_baz", ct::re<R"(_)">::remove>;
std::string_view{x};            // "foobarbaz"
```

### Chaining with `fmt::chain`

Works with other transforms in a pipeline:

```cpp
auto x = ct::str<"  hello   world  ", ct::fmt::chain<
    ct::fmt::trim_each_line,
    ct::re<R"(\s+)">::replace<" ">
>>;
std::string_view{x};            // "hello world"
```

### `constexpr` context

```cpp
constexpr auto x = ct::str<"a--b--c", ct::re<R"(--)">::replace<".">>;
static_assert( std::string_view{x} == "a.b.c" );
```

### Availability

`ct::re` is only defined when `<ctre.hpp>` is available. If you need
compile-time regex but CTRE is not available, use `ct::fmt::replace_all` for
fixed-string replacements, or add CTRE to your project dependencies.

