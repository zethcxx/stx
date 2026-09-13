#pragma once

#include "lbyte/stx/core.hpp"
#include "lbyte/stx/mem.hpp"
#include "lbyte/stx/arr.hpp"

#include <compare>
#include <concepts>
#include <cstddef>
#include <cstring>
#include <span>
#include <tuple>
#include <type_traits>
#include <utility>

// ===========================================================================
// ct::record / ct::member / ct::attr
//
// A compile-time key -> type descriptor. `record` is an EMPTY type: it holds
// no data and materializes nothing at runtime. Every query (size, offsets,
// alignment, lookup, iteration) is constexpr / compile-time, so you only pay
// for what you actually use:
//
//     using hdr = ct::record<
//         ct::member<f::magic, u32>,
//         ct::member<f::code,  u16, ct::attr::align<4>>,
//         ct::member<f::tail,  char, ct::attr::packed>
//     >;
//
//     constexpr usize n = hdr::byte_total;        // the "sizeof" of the layout
//     auto v = cur.pop<hdr>();                    // read a whole record "as a struct"
//
// `member` value types must be trivially copyable; keys are an enum, a
// byte-offset newtype (off_s / rva_s / user tags), or a string literal via
// `ct::smember<"name", T>` (a compile-time `key_str` non-type parameter - no
// enum to declare, no runtime strings, renames are not tracked by tooling).
// Attributes:
//
//     ct::attr::packed     -> skip alignment for this member / whole record
//     ct::attr::align<N>   -> force this member / trailing pad to alignment N
//     ct::attr::gap<N>     -> N bytes of explicit padding before a member
//
// The value of a record is a std::tuple (hdr::value_t) of the member types.
// Read it from bytes with ct::load<rec>(ptr), write it with ct::store<rec>,
// or let memcur/ptr do it: cur.pop<rec>() / p.read<rec>().
//
// To walk raw bytes without copying (read or mutate in place, and iterate
// member key / offset / size), build a zero-copy `ct::record_view<rec>` over
// the buffer:
//
//     ct::record_view<hdr> r{ buf.data() };
//     for ( auto m : r ) { m.key; m.offset; m.size; }   // walk members
//     r.get<"flags">()        // zero copy, mutates the buffer
// ===========================================================================

namespace lbyte::stx::ct
{
    // forward declarations (defined below record_ops)
    template<typename Rec> struct loader;
    template<typename Rec> struct store_impl;

    namespace attr
    {
        struct packed
        {
            static constexpr usize value = 1;
        };

        template<usize N>
        struct align
        {
            static_assert( N >= 2 && ( N & ( N - 1 )) == 0,
                "ct::attr::align<N>: N must be a power of two (>= 2)" );
            static constexpr usize value = N;
        };

        template<usize N>
        struct gap
        {
            static_assert( N > 0, "ct::attr::gap<N>: N must be > 0" );
            static constexpr usize value = N;
        };
    }

    // ---- key_str --------------------------------------------------------
    // A compile-time string key for `member`: a fixed-capacity literal type
    // that works as a non-type template parameter. `smember<"count", u16>`
    // builds one from the string literal; every member of a record then
    // shares the exact same key type, keeping the record's key_type
    // homogeneous. A name that does not fit the capacity fails at compile
    // time. It is a compile-time value: there are no runtime strings.

    template<usize Cap = 32>
    struct key_str
    {
        static_assert( Cap > 1, "ct::key_str<Cap>: Cap must be > 1" );

        char data[Cap]{};

        constexpr key_str() noexcept = default;

        template<usize M>
        constexpr key_str( const char (&s)[M] ) noexcept
        {
            static_assert( M <= Cap,
                "ct::key_str<Cap>: string literal is longer than the capacity" );
            for ( usize i = 0; i < M; ++i )
                data[i] = s[i];
        }

        [[nodiscard]] constexpr usize size() const noexcept
        {
            usize n = 0;
            while ( n < Cap && data[n] != '\0' ) ++n;
            return n;
        }

        [[nodiscard]] constexpr const char* c_str() const noexcept { return data; }

        friend constexpr bool operator==( const key_str& a, const key_str& b ) noexcept
        {
            for ( usize i = 0; i < Cap; ++i )
                if ( a.data[i] != b.data[i] ) return false;
            return true;
        }

        friend constexpr auto operator<=>( const key_str& a, const key_str& b ) noexcept
        {
            for ( usize i = 0; i < Cap; ++i )
            {
                if ( a.data[i] < b.data[i] ) return std::strong_ordering::less;
                if ( a.data[i] > b.data[i] ) return std::strong_ordering::greater;
            }
            return std::strong_ordering::equal;
        }
    };

    template<typename T> struct key_str_t : std::false_type {};
    template<usize Cap> struct key_str_t<key_str<Cap>> : std::true_type {};
    template<typename T> inline constexpr bool is_key_str =
        key_str_t<std::remove_cvref_t<T>>::value;

    // ---- record_key --------------------------------------------------------
    // Any value usable as a `member` key: an enum class, a byte-offset
    // newtype (off_s / rva_s / user tag opting into `is_offset_tag`), or a
    // `key_str` built from a string literal.

    template<typename K>
    concept record_key
        =  std::is_enum_v<std::remove_cvref_t<K>>
        or byte_offset<std::remove_cvref_t<K>>
        or is_key_str<K>;

    // keyed member (enum / byte-offset) and string-keyed member, forward
    // declared so the collectors below can match their patterns.
    template<record_key auto Key, typename ValueT, typename... Attrs>
        requires ( std::is_trivially_copyable_v<ValueT> )
    struct member;

    template<key_str<32> Key, typename ValueT, typename... Attrs>
        requires ( std::is_trivially_copyable_v<ValueT> )
    struct smember;

    namespace detail
    {
        // --- attribute extraction ------------------------------------------

        template<typename A> struct is_packed_attr_t : std::false_type {};
        template<> struct is_packed_attr_t<attr::packed> : std::true_type {};
        template<typename A> inline constexpr bool is_packed_attr = is_packed_attr_t<A>::value;

        template<typename A> struct align_attr_t : std::integral_constant<usize, 0> {};
        template<usize N> struct align_attr_t<attr::align<N>> : std::integral_constant<usize, N> {};
        template<typename A> inline constexpr usize align_attr_of = align_attr_t<A>::value;

        template<typename A> struct gap_attr_t : std::integral_constant<usize, 0> {};
        template<usize N> struct gap_attr_t<attr::gap<N>> : std::integral_constant<usize, N> {};
        template<typename A> inline constexpr usize gap_attr_of = gap_attr_t<A>::value;

        template<typename A>
        inline constexpr bool is_attr_v
            =  is_packed_attr<A>
            or align_attr_of<A> != 0
            or gap_attr_of<A> != 0;

        template<typename... A> struct max_align_attr;
        template<> struct max_align_attr<> : std::integral_constant<usize, 0> {};
        template<typename A0, typename... Rest>
        struct max_align_attr<A0, Rest...>
        {
            static constexpr usize head = align_attr_of<A0>;
            static constexpr usize tail = max_align_attr<Rest...>::value;
            static constexpr usize value = head > tail ? head : tail;
        };

        template<typename... A>
        inline constexpr usize gap_total = usize{0} + ( gap_attr_of<A> + ... + usize{0} );

        // Layout-relevant facts shared by `member` and `smember`.
        template<typename ValueT, typename... Attrs>
        struct member_shape
        {
            static_assert( ( is_attr_v<Attrs> and ... ),
                "ct::member<Key, T, Attrs...>: each attr must be ct::attr::{packed, align<N>, gap<N>}" );

            using value_type = ValueT;

            // requested alignment: attr::align<N> wins, else natural alignment
            static constexpr usize align
                = alignof( ValueT ) > max_align_attr<Attrs...>::value
                    ? alignof( ValueT ) : max_align_attr<Attrs...>::value;

            // bytes of explicit padding placed before this member
            static constexpr usize gap = gap_total<Attrs...>;

            static constexpr bool is_packed = ( is_packed_attr<Attrs> or ... );
        };

        template<usize... Vs> struct max_usize_list;
        template<> struct max_usize_list<> : std::integral_constant<usize, 1> {};
        template<usize V0, usize... Rest>
        struct max_usize_list<V0, Rest...>
        {
            static constexpr usize tail = max_usize_list<Rest...>::value;
            static constexpr usize value = V0 > tail ? V0 : tail;
        };

        [[nodiscard]] constexpr usize align_up( usize value, usize alignment ) noexcept
        {
            if ( alignment <= 1 ) return value;
            return ( value + alignment - 1 ) / alignment * alignment;
        }

        // --- member detection / collection ----------------------------------

        template<typename T> struct is_member_t : std::false_type {};
        template<record_key auto Key, typename T, typename... A>
        struct is_member_t<member<Key, T, A...>> : std::true_type {};
        template<key_str<32> Key, typename T, typename... A>
        struct is_member_t<smember<Key, T, A...>> : std::true_type {};
        template<typename T> inline constexpr bool is_member = is_member_t<T>::value;

        template<typename... Acc> struct collect_members;
        template<typename... Acc> struct collect_members<std::tuple<Acc...>>
        {
            using type = std::tuple<Acc...>;
        };
        template<typename... Acc, typename Head, typename... Rest>
        struct collect_members<std::tuple<Acc...>, Head, Rest...>
        {
            using type = std::conditional_t<
                is_member<Head>,
                typename collect_members<std::tuple<Acc..., Head>, Rest...>::type,
                typename collect_members<std::tuple<Acc...>, Rest...>::type>;
        };

        // Standalone `attr::gap<N>` items between members become padding for the
        // member that follows them: each member is rebuilt with an extra
        // `attr::gap<P>` attribute carrying its accumulated gap bytes.
        template<typename Tuple, usize Pending, typename... Items>
        struct collect_gapped;
        template<typename Tuple, usize Pending>
        struct collect_gapped<Tuple, Pending> { using type = Tuple; };
        template<typename Tuple, usize Pending, typename Head, typename... Rest>
        struct collect_gapped<Tuple, Pending, Head, Rest...>
        {
            static constexpr bool head_is_gap = gap_attr_of<Head> != 0 and not is_member<Head>;
            static constexpr usize next_pending = head_is_gap ? Pending + gap_attr_of<Head> : Pending;

            template<usize P, typename M>
            struct rebuild;
            template<usize P, record_key auto Key, typename T, typename... A>
            struct rebuild<P, member<Key, T, A...>>
            {
                using type = std::conditional_t<
                    P == 0,
                    std::tuple<member<Key, T, A...>>,
                    std::tuple<member<Key, T, A..., attr::gap<P>>>>;
            };
            template<usize P, key_str<32> Key, typename T, typename... A>
            struct rebuild<P, smember<Key, T, A...>>
            {
                using type = std::conditional_t<
                    P == 0,
                    std::tuple<smember<Key, T, A...>>,
                    std::tuple<smember<Key, T, A..., attr::gap<P>>>>;
            };

            template<bool IsMember, typename Tup, usize Pend, typename H>
            struct maybe_append { using type = Tup; };
            template<typename Tup, usize Pend, typename H>
            struct maybe_append<true, Tup, Pend, H>
            {
                using type = decltype( std::tuple_cat(
                    std::declval<Tup>(),
                    std::declval<typename rebuild<Pend, H>::type>() ));
            };

            using appended = typename maybe_append<is_member<Head>, Tuple, next_pending, Head>::type;

            using type = typename collect_gapped<
                appended,
                is_member<Head> ? usize{0} : next_pending,
                Rest...>::type;
        };

        template<typename... Items>
        using member_tuple_t = typename collect_gapped<std::tuple<>, 0, Items...>::type;

        template<typename... Rest> struct check_items;
        template<> struct check_items<> { static constexpr bool value = true; };
        template<typename Head, typename... Rest>
        struct check_items<Head, Rest...>
        {
            static constexpr bool value = ( is_member<Head> || is_attr_v<Head> )
                && check_items<Rest...>::value;
        };

        // ---- record core ----------------------------------------------------

        template<auto A, auto B>
        inline constexpr bool keys_equal = ( A == B );

        template<auto Key, typename... MS>
        struct find_key;
        template<auto Key>
        struct find_key<Key> { using type = void; };
        template<auto Key, typename M, typename... MS>
        struct find_key<Key, M, MS...>
        {
            using type = std::conditional_t<
                keys_equal<Key, M::key>,
                M,
                typename find_key<Key, MS...>::type>;
        };

        template<auto Key, typename M>
        constexpr bool member_match = keys_equal<Key, M::key>;

        // Duplicate detection by content equality (key_str ==, enum ==,
        // offset ==) instead of a numeric hash, so string keys work too.
        template<typename... MS> struct all_distinct_t;
        template<> struct all_distinct_t<> { static constexpr bool value = true; };
        template<typename Head, typename... Rest>
        struct all_distinct_t<Head, Rest...>
        {
            static constexpr bool value = ( ( !member_match<Head::key, Rest> ) and ... )
                                          and all_distinct_t<Rest...>::value;
        };

        template<bool RecPacked, usize RecAlign, typename... MS>
        struct record_ops
        {
            static constexpr usize count = sizeof...( MS );

            using member_types = std::tuple<MS...>;
            using value_t      = std::tuple<typename MS::value_type...>;
            using key_type     = std::remove_cvref_t<decltype( std::tuple_element_t<0, member_types>::key )>;

            static_assert(
                ( std::same_as<key_type,
                        std::remove_cvref_t<decltype( MS::key )>> and ... ),
                "ct::record: all member keys must share the same type" );

            static constexpr bool all_distinct = detail::all_distinct_t<MS...>::value;
            static_assert( all_distinct, "ct::record: duplicate member keys" );

            // ---- layout math -------------------------------------------------

            static constexpr usize member_max_align
                = max_usize_list< (MS::is_packed ? usize{1} : MS::align)... >::value;

            static constexpr usize trailing_align = RecAlign != 0 ? RecAlign : member_max_align;

            static constexpr usize max_align = max_usize_list<RecAlign, (MS::is_packed ? usize{1} : MS::align)...>::value;

            [[nodiscard]] static consteval ::lbyte::stx::arr<usize, count> make_offsets( bool packed_all )
            {
                ::lbyte::stx::arr<usize, count> out{};
                usize cur = 0;
                usize i   = 0;
                ( [&] {
                    cur += MS::gap;
                    usize al = ( MS::is_packed || packed_all ) ? 1 : MS::align;
                    cur = detail::align_up( cur, al );
                    out[i] = cur;
                    ++i;
                    cur += sizeof( typename MS::value_type );
                }(), ... );
                return out;
            }

            [[nodiscard]] static consteval usize total_size( bool packed_all )
            {
                usize cur = 0;
                ( [&] {
                    cur += MS::gap;
                    usize al = ( MS::is_packed || packed_all ) ? 1 : MS::align;
                    cur = detail::align_up( cur, al );
                    cur += sizeof( typename MS::value_type );
                }(), ... );
                if ( !packed_all )
                    cur = detail::align_up( cur, trailing_align );
                return cur;
            }

            static constexpr ::lbyte::stx::arr<usize, count> offsets        = make_offsets( RecPacked );
            static constexpr ::lbyte::stx::arr<usize, count> packed_offsets = make_offsets( true );

            static constexpr usize byte_total   = total_size( RecPacked );
            static constexpr usize packed_total = total_size( true );

            // ---- reader fingerprint ------------------------------------------
            // Lets the generic ptr/memcur ops (details::record_like / read_value
            // in mem.hpp) read this record "as a struct" through the mixin,
            // without any reader_of/is_record specialization in mem/io.

            struct reader
            {
                using value_t = record_ops::value_t;
                static constexpr usize byte_size = record_ops::byte_total;

                [[nodiscard]] static constexpr value_t read( const ::lbyte::stx::uptr addr ) noexcept
                {
                    return ::lbyte::stx::ct::loader<record_ops>::run(
                        ::lbyte::stx::rcast<const std::byte*>( addr ) );
                }
            };

            // ---- lookup -------------------------------------------------------

            template<auto Key>
            static constexpr bool has = ( detail::member_match<Key, MS> or ... );

            template<auto Key>
            struct value_of_t
            {
                static_assert( record_ops::has<Key>,
                    "ct::record::value_of: key not in record" );
                using type = typename detail::find_key<Key, MS...>::type::value_type;
            };
            template<auto Key>
            using value_of = typename value_of_t<Key>::type;

            // Plain lookup takes any `record_key` value (enum, offset, or a key_str
            // built from a literal). The `key_str<32>` overloads let callers
            // hand the string literal directly:
            //     rec::index_of<sec::crc>()    rec::index_of<"crc">()

            template<record_key auto Key>
            [[nodiscard]] static constexpr usize index_of_impl() noexcept
            {
                static_assert( record_ops::has<Key>,
                    "ct::record::index_of: key not in record" );
                usize i = 0, out = 0;
                bool found = false;
                ( [&] {
                    if constexpr ( detail::member_match<Key, MS> )
                    {
                        if ( !found ) { out = i; found = true; }
                    }
                    ++i;
                }(), ... );
                return out;
            }

            template<record_key auto Key>
            [[nodiscard]] static constexpr usize index_of() noexcept
            {
                return index_of_impl<Key>();
            }

            template<key_str<32> Key>
            [[nodiscard]] static constexpr usize index_of() noexcept
            {
                return index_of_impl<Key>();
            }

            template<record_key auto Key>
            [[nodiscard]] static constexpr usize offset_of_impl() noexcept
            {
                return offsets[ index_of_impl<Key>() ];
            }

            template<record_key auto Key>
            [[nodiscard]] static constexpr usize offset_of() noexcept
            {
                return offset_of_impl<Key>();
            }

            template<key_str<32> Key>
            [[nodiscard]] static constexpr usize offset_of() noexcept
            {
                return offset_of_impl<Key>();
            }

            template<typename T>
            [[nodiscard]] static constexpr key_type key_of() noexcept
            {
                static_assert( ( std::same_as<T, typename MS::value_type> or ... ),
                    "ct::record::key_of: no member with that value type" );
                key_type out{};
                bool found = false;
                ( [&] {
                    if constexpr ( std::same_as<T, typename MS::value_type> )
                    {
                        if ( !found ) { out = MS::key; found = true; }
                    }
                }(), ... );
                return out;
            }

            // ---- typed access by key -------------------------------------------
            // Map-like `rec[key]` with a per-key return type needs reflection and
            // is not expressible in C++23. `get<Key>(value_t)` is the analogue:
            // (static) key, typed member reference. A string literal is accepted
            // the same way (it is normalized to a key_str value, still compile
            // time): rec::get<sec::crc>(v)    rec::get<"crc">(v)

            template<record_key auto Key>
            [[nodiscard]] static constexpr auto& get_impl( value_t& value ) noexcept
            {
                static_assert( has<Key>, "ct::record::get: key not in record" );
                return std::get< index_of_impl<Key>() >( value );
            }

            template<record_key auto Key>
            [[nodiscard]] static constexpr const auto& get_impl( const value_t& value ) noexcept
            {
                static_assert( has<Key>, "ct::record::get: key not in record" );
                return std::get< index_of_impl<Key>() >( value );
            }

            template<record_key auto Key>
            [[nodiscard]] static constexpr auto& get( value_t& value ) noexcept
            {
                return get_impl<Key>( value );
            }

            template<record_key auto Key>
            [[nodiscard]] static constexpr const auto& get( const value_t& value ) noexcept
            {
                return get_impl<Key>( value );
            }

            template<key_str<32> Key>
            [[nodiscard]] static constexpr auto& get( value_t& value ) noexcept
            {
                return get_impl<Key>( value );
            }

            template<key_str<32> Key>
            [[nodiscard]] static constexpr const auto& get( const value_t& value ) noexcept
            {
                return get_impl<Key>( value );
            }

            // ---- functional iteration ------------------------------------------

            template<typename F>
            static constexpr void visit( F&& f ) noexcept
            {
                ( std::forward<F>( f )( MS{} ), ... );
            }

            template<typename R, typename F>
            static constexpr R fold( R init, F&& f ) noexcept
            {
                ( ( init = std::forward<F>( f )( MS{}, init ) ), ... );
                return init;
            }

            // ---- homogeneous descriptor tables ----------------------------------
            // All keys share one type, so keys/sizes/meta are plain arrays:
            // range-`for` and `[]` work at runtime (typed per-member access does
            // not - that is what `visit`/`fold`/`std::get` are for).

            static constexpr ::lbyte::stx::arr<key_type,     count> keys{ MS::key... };
            static constexpr ::lbyte::stx::arr<usize,        count> sizes{ sizeof( typename MS::value_type )... };

            struct member_meta
            {
                key_type key;
                usize    offset;
                usize    size;

                constexpr bool operator==( const member_meta& ) const = default;
            };

            static constexpr ::lbyte::stx::arr<member_meta, count> meta = [] {
                ::lbyte::stx::arr<member_meta, count> m{};
                for ( usize i = 0; i < count; ++i )
                    m[i] = member_meta{ keys[i], offsets[i], sizes[i] };
                return m;
            }();

            [[nodiscard]] static constexpr const member_meta& meta_of( key_type key ) noexcept
            {
                for ( usize i = 0; i < count; ++i )
                    if ( meta[i].key == key ) return meta[i];
                return meta[0];
            }

            // ---- iteration ------------------------------------------------------
            // Typed per-member iteration is `visit`/`fold` (each member's type is
            // distinct, so a runtime `for` cannot re-type per index without
            // reflection). `members` is the heterogeneous tuple of member values
            // for std::get / structured bindings / std::apply.

            static constexpr std::tuple<MS...> members{};
        };

        template<bool RecPacked, usize RecAlign, typename Tuple>
        struct make_record_core;
        template<bool RecPacked, usize RecAlign, typename... MS>
        struct make_record_core<RecPacked, RecAlign, std::tuple<MS...>>
        {
            using type = record_ops<RecPacked, RecAlign, MS...>;
        };

        template<typename... Items>
        struct packed_flag
        {
            static constexpr bool value = ( is_packed_attr<Items> or ... );
        };

        template<typename... Items>
        struct align_at
        {
            static constexpr usize value = max_align_attr<Items...>::value;
        };
    }

    // ---- member / smember -----------------------------------------------------
    // `member` takes an enum / byte-offset key, `smember` a string-literal
    // key (`smember<"count", u16>`); both share the same shape and attributes.

    template<record_key auto Key, typename ValueT, typename... Attrs>
        requires ( std::is_trivially_copyable_v<ValueT> )
    struct member : detail::member_shape<ValueT, Attrs...>
    {
        static constexpr auto key = Key;
    };

    template<key_str<32> Key, typename ValueT, typename... Attrs>
        requires ( std::is_trivially_copyable_v<ValueT> )
    struct smember : detail::member_shape<ValueT, Attrs...>
    {
        static constexpr key_str<32> key = Key;
    };

    // ---- record ---------------------------------------------------------------

    template<typename... Items>
    struct record
        : detail::make_record_core<
              detail::packed_flag<Items...>::value,
              detail::align_at<Items...>::value,
              detail::member_tuple_t<Items...>>::type
    {
        static_assert( detail::check_items<Items...>::value,
            "ct::record<...>: items must be ct::member<...> or ct::attr::{packed, align<N>}" );
    };

    template<typename Rec>
    using record_value_of_t = typename Rec::value_t;

    // ---- load / store -------------------------------------------------------------
    // Read a record from raw bytes into its value tuple, and write it back.
    // The layout used is `Rec::offsets` / `Rec::byte_total`, so attributes
    // (alignment, packing, gaps) are honored exactly like a struct would.

    template<typename Rec>
    struct loader
    {
        static constexpr usize count = Rec::count;

        template<usize I, typename V>
        static constexpr void one( const std::byte* p, V& out ) noexcept
        {
            using T = std::tuple_element_t<I, V>;
            std::memcpy( std::addressof( std::get<I>( out ) ), p + Rec::offsets[I], sizeof( T ) );
        }

        [[nodiscard]] static constexpr record_value_of_t<Rec> run( const std::byte* p ) noexcept
        {
            record_value_of_t<Rec> out{};
            [&]<usize... Is>( std::index_sequence<Is...> ) {
                ( one<Is>( p, out ), ... );
            }( std::make_index_sequence<count>{} );
            return out;
        }
    };

    template<typename Rec>
    struct store_impl
    {
        static constexpr usize count = Rec::count;

        template<usize I, typename V>
        static constexpr void one( std::byte* p, const V& value ) noexcept
        {
            using T = std::tuple_element_t<I, V>;
            std::memcpy( p + Rec::offsets[I], std::addressof( std::get<I>( value ) ), sizeof( T ) );
        }

        static constexpr void run( std::byte* p, const record_value_of_t<Rec>& value ) noexcept
        {
            [&]<usize... Is>( std::index_sequence<Is...> ) {
                ( one<Is>( p, value ), ... );
            }( std::make_index_sequence<count>{} );
        }
    };

    template<typename Rec>
        requires ( ::lbyte::stx::details::record_like<Rec> )
    [[nodiscard]] constexpr record_value_of_t<Rec> load( const void* src ) noexcept
    {
        return loader<Rec>::run( rcast<const std::byte*>( src ) );
    }

    template<typename Rec>
        requires ( ::lbyte::stx::details::record_like<Rec> )
    [[nodiscard]] constexpr record_value_of_t<Rec> load( std::span<const std::byte> src ) noexcept
    {
        return load<Rec>( src.data() );
    }

    template<typename Rec, typename Value>
        requires ( ::lbyte::stx::details::record_like<Rec> )
    constexpr void store( std::byte* dst, const Value& value ) noexcept
    {
        static_assert( std::same_as<std::remove_cvref_t<Value>, record_value_of_t<Rec>>,
            "ct::store<rec>: value must be the record's value tuple (rec::value_t)" );
        store_impl<Rec>::run( dst, value );
    }

    template<typename Rec, typename Value>
        requires ( ::lbyte::stx::details::record_like<Rec> )
    constexpr void store( std::span<std::byte> dst, const Value& value ) noexcept
    {
        store<Rec>( dst.data(), value );
    }

    // ---- record_view -----------------------------------------------------------
    // A zero-copy window over the raw bytes a record describes. It never
    // copies: `get<Key>()` / `get<"name">()` return references straight into
    // the buffer (mutating them changes the buffer), and the iterators walk
    // the constexpr member table (key / offset / size) without touching a
    // single member.

    template<typename Rec>
        requires ( ::lbyte::stx::details::record_like<Rec> )
    class record_view
    {
        std::byte* base_{};

    public:
        using record_type = Rec;
        using member_meta = typename Rec::member_meta;

        constexpr record_view() noexcept = default;
        constexpr record_view( void* base ) noexcept
            : base_( scast<std::byte*>( base ) ) {}
        constexpr record_view( ::lbyte::stx::uptr addr ) noexcept
            : base_( rcast<std::byte*>( addr ) ) {}
        constexpr record_view( const record_view& ) noexcept = default;
        constexpr record_view& operator=( const record_view& ) noexcept = default;

        constexpr bool operator==( const record_view& ) const noexcept = default;

        [[nodiscard]] static constexpr usize count() noexcept { return Rec::count; }
        [[nodiscard]] static constexpr usize byte_count() noexcept { return Rec::byte_total; }
        [[nodiscard]] constexpr std::byte* data() const noexcept { return base_; }

        // ---- descriptor walk (no bytes touched) --------------------------------

        [[nodiscard]] constexpr const typename Rec::member_meta& meta_of( usize i ) const noexcept
        {
            return Rec::meta[ i ];
        }

        [[nodiscard]] constexpr usize offset_of( usize i ) const noexcept { return Rec::offsets[ i ]; }
        [[nodiscard]] constexpr usize size_of(   usize i ) const noexcept { return Rec::sizes[ i ]; }
        [[nodiscard]] constexpr typename Rec::key_type key_of( usize i ) const noexcept
        {
            return Rec::keys[ i ];
        }

        // ---- typed access into the buffer (zero copy) --------------------------

        template<record_key auto Key>
        [[nodiscard]] constexpr const typename Rec::template value_of<Key>&
            get_typed() const noexcept
        {
            return *rcast<const typename Rec::template value_of<Key>*>(
                base_ + Rec::template offset_of_impl<Key>() );
        }

        template<record_key auto Key>
        [[nodiscard]] constexpr typename Rec::template value_of<Key>& get_typed() noexcept
        {
            return *rcast<typename Rec::template value_of<Key>*>(
                base_ + Rec::template offset_of_impl<Key>() );
        }

        template<record_key auto Key>
        [[nodiscard]] constexpr const typename Rec::template value_of<Key>& get() const noexcept
        {
            return get_typed<Key>();
        }

        template<record_key auto Key>
        [[nodiscard]] constexpr typename Rec::template value_of<Key>& get() noexcept
        {
            return get_typed<Key>();
        }

        template<key_str<32> Key>
        [[nodiscard]] constexpr const typename Rec::template value_of<Key>& get() const noexcept
        {
            return get_typed<Key>();
        }

        template<key_str<32> Key>
        [[nodiscard]] constexpr typename Rec::template value_of<Key>& get() noexcept
        {
            return get_typed<Key>();
        }

        template<usize I>
        [[nodiscard]] constexpr const std::tuple_element_t<I, typename Rec::value_t>&
            at() const noexcept
        {
            using T = std::tuple_element_t<I, typename Rec::value_t>;
            return *rcast<const T*>( base_ + Rec::offsets[ I ] );
        }

        template<usize I>
        [[nodiscard]] constexpr std::tuple_element_t<I, typename Rec::value_t>& at() noexcept
        {
            using T = std::tuple_element_t<I, typename Rec::value_t>;
            return *rcast<T*>( base_ + Rec::offsets[ I ] );
        }

        // ---- iteration -----------------------------------------------------------

        class iterator
        {
            const record_view* self_{};
            usize i_{};

        public:
            using value_type = typename Rec::member_meta;

            constexpr iterator() noexcept = default;
            constexpr iterator( const record_view* self, usize i ) noexcept
                : self_( self ), i_( i ) {}

            [[nodiscard]] constexpr usize index() const noexcept { return i_; }

            [[nodiscard]] constexpr value_type operator*() const noexcept
            {
                return Rec::meta[ i_ ];
            }

            constexpr iterator& operator++() noexcept { ++i_; return *this; }
            constexpr iterator operator++( int ) noexcept { auto c = *this; ++i_; return c; }

            friend constexpr bool operator==( const iterator&, const iterator& ) noexcept = default;
            friend constexpr bool operator!=( const iterator&, const iterator& ) noexcept = default;
        };

        [[nodiscard]] constexpr iterator begin() const noexcept { return { this, 0 }; }
        [[nodiscard]] constexpr iterator end()   const noexcept { return { this, Rec::count }; }
        [[nodiscard]] static constexpr bool empty() noexcept { return Rec::count == 0; }
    };

    template<typename Rec>
        requires ( ::lbyte::stx::details::record_like<Rec> )
    [[nodiscard]] constexpr record_view<Rec> view( void* src ) noexcept
    {
        return record_view<Rec>( src );
    }

    template<typename Rec>
        requires ( ::lbyte::stx::details::record_like<Rec> )
    [[nodiscard]] constexpr record_view<Rec> view( ::lbyte::stx::uptr addr ) noexcept
    {
        return record_view<Rec>( addr );
    }
}