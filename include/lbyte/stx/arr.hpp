#pragma once

#include "core.hpp"

#include <array>
#include <compare>
#include <cstddef>
#include <initializer_list>
#include <utility>

namespace lbyte::stx
{
    // ---- index key -----------------------------------------------------------
    // A value usable as a keyed index into arr<T, N>: any enum class, or a
    // byte-offset newtype (see byte_offset / offset_s). Raw integrals are
    // handled by the operator[](size_type) overload, so they are intentionally
    // excluded from arr_key to avoid overload ambiguity.
    //
    // A byte-offset key is an ELEMENT index (same as an integral): sizeof(T)
    // is not factored in. Only for byte-sized elements (sizeof(T)==1) does the
    // element index coincide with the byte offset.

    template<typename K>
    concept arr_key
        =  std::is_enum_v<std::remove_cvref_t<K>>
        or byte_offset<std::remove_cvref_t<K>>;

    namespace details
    {
        template<arr_key K>
        [[nodiscard]] constexpr usize arr_index( K key ) noexcept
        {
            using U = std::remove_cvref_t<K>;
            if constexpr ( std::is_enum_v<U> )
                return static_cast<usize>( std::to_underlying( key ) );
            else // byte_offset newtype: used as element index, not byte offset
                return static_cast<usize>( key.get() );
        }
    }

    // ---- arr -----------------------------------------------------------------
    // A contiguously-stored fixed-size array: a modern, std::array-compatible
    // refit. Same layout / zero-cost, but indexable by enum or byte-offset
    // newtype in addition to raw integrals, with a few renamed convenience
    // members (e.g. is_empty) and a std::to_array-style arr_of factory.
    //
    // `arr` holds a std::array member (staying an aggregate so brace-init and
    // constexpr work); std::array itself cannot be inherited as an aggregate.
    //
    // Initialization follows std::array exactly:
    //   * `arr<T,N> a{}`     -> value-initialization zero-fills the elements;
    //   * `arr<T,N> a{...}`  -> brace-initialized from the given elements;
    //   * `arr<T,N> a;`      -> default-initialization leaves the elements
    //                           uninitialized (no zeroing cycle is spent).
    // A subclass that never touches the backing storage is `stx::dirty_arr`
    // (see below): the same interface, but cheap for a buffer that is about
    // to be overwritten completely.

    template<typename Type, usize N>
    struct arr
    {
        using base_array = std::array<Type, N>;

        using value_type      = typename base_array::value_type;
        using size_type       = typename base_array::size_type;
        using difference_type = typename base_array::difference_type;
        using reference       = typename base_array::reference;
        using const_reference = typename base_array::const_reference;
        using pointer         = typename base_array::pointer;
        using const_pointer   = typename base_array::const_pointer;
        using iterator        = typename base_array::iterator;
        using const_iterator  = typename base_array::const_iterator;

        base_array elems;

        // ---- element access --------------------------------------------------

        [[nodiscard]] constexpr reference       operator[]( size_type i )       noexcept { return elems[i]; }
        [[nodiscard]] constexpr const_reference operator[]( size_type i ) const noexcept { return elems[i]; }

        template<arr_key K>
        [[nodiscard]] constexpr reference operator[]( K key ) noexcept
        {
            return elems[ details::arr_index( key ) ];
        }

        template<arr_key K>
        [[nodiscard]] constexpr const_reference operator[]( K key ) const noexcept
        {
            return elems[ details::arr_index( key ) ];
        }

        [[nodiscard]] constexpr reference       at( size_type i )       { return elems.at( i ); }
        [[nodiscard]] constexpr const_reference at( size_type i ) const { return elems.at( i ); }

        [[nodiscard]] constexpr reference       front()       noexcept { return elems.front(); }
        [[nodiscard]] constexpr const_reference front() const noexcept { return elems.front(); }
        [[nodiscard]] constexpr reference       back()        noexcept { return elems.back(); }
        [[nodiscard]] constexpr const_reference back()  const noexcept { return elems.back(); }

        [[nodiscard]] constexpr pointer       data()       noexcept { return elems.data(); }
        [[nodiscard]] constexpr const_pointer data() const noexcept { return elems.data(); }

        // ---- iterators -------------------------------------------------------

        [[nodiscard]] constexpr iterator       begin()       noexcept { return elems.begin(); }
        [[nodiscard]] constexpr const_iterator begin() const noexcept { return elems.begin(); }
        [[nodiscard]] constexpr iterator       end()         noexcept { return elems.end(); }
        [[nodiscard]] constexpr const_iterator end()   const noexcept { return elems.end(); }
        [[nodiscard]] constexpr const_iterator cbegin() const noexcept { return elems.cbegin(); }
        [[nodiscard]] constexpr const_iterator cend()   const noexcept { return elems.cend(); }

        [[nodiscard]] constexpr auto rbegin()       noexcept { return elems.rbegin(); }
        [[nodiscard]] constexpr auto rbegin() const noexcept { return elems.rbegin(); }
        [[nodiscard]] constexpr auto rend()         noexcept { return elems.rend(); }
        [[nodiscard]] constexpr auto rend()   const noexcept { return elems.rend(); }
        [[nodiscard]] constexpr auto crbegin() const noexcept { return elems.crbegin(); }
        [[nodiscard]] constexpr auto crend()   const noexcept { return elems.crend(); }

        // ---- capacity --------------------------------------------------------

        [[nodiscard]] constexpr size_type size()     const noexcept { return elems.size(); }
        [[nodiscard]] constexpr size_type max_size() const noexcept { return elems.max_size(); }
        [[nodiscard]] constexpr bool      is_empty() const noexcept { return elems.empty(); }

        // ---- operations ------------------------------------------------------

        constexpr void fill( const Type& value ) noexcept { elems.fill( value ); }
        constexpr void swap( arr& other ) noexcept( std::is_nothrow_swappable_v<base_array> )
        {
            elems.swap( other.elems );
        }

        // ---- comparisons -----------------------------------------------------

        [[nodiscard]] constexpr friend auto operator<=>( const arr&, const arr& ) = default;
    };

    // ---- CTAD from element pack ----------------------------------------------
    // Mirrors std::array's explicit deduction guide: `arr a{1, 2, 3}` deduces
    // Type = int and N = 3 (all elements must share the element type).

    template<typename Type, typename... Us>
        requires ( std::same_as<Type, Us> and ... )
    arr( Type, Us... ) -> arr<Type, 1 + sizeof...( Us )>;

    // ---- dirty_arr -----------------------------------------------------------
    // An arr<T, N> whose default constructor does NOT touch the backing array:
    // the elements start with indeterminate values. Use it for a fixed-size
    // buffer that is filled entirely before being read, so no clock cycles are
    // wasted zero-initializing it (same idea as io::dirty_vector).
    //
    //     alignas(16) auto batch_buff = dirty_arr<char, batch_buffer_size>{};
    //
    // Everything else is inherited from arr<T, N>: indexing (integral, enum,
    // byte-offset key), data(), iterators, size(), fill(), swap(), ordering.

    template<typename Type, usize N>
    struct dirty_arr : arr<Type, N>
    {
        using base = arr<Type, N>;

        constexpr dirty_arr() noexcept {}                            // leaves elems untouched

        constexpr explicit dirty_arr( const base&  src ) noexcept : base( src ) {}
        constexpr explicit dirty_arr( base&& src ) noexcept : base( std::move( src ) ) {}

        // Re-export the base's comparison operators for dirty_arr itself.
        [[nodiscard]] constexpr friend auto operator<=>( const dirty_arr&, const dirty_arr& ) = default;
    };

    // ---- arr_of factory (std::to_array style) --------------------------------
    // Deduces Type and N from a brace array: `arr_of<T>({...})` (rvalue temp)
    // and `arr_of({...})` both work. An lvalue C-array overload accepts named
    // arrays, which is what lets C array-designators `[idx] = v` (valid only in
    // a C-array, not in a class) seed a typed arr in a single constexpr:
    //     inline constexpr u64 raw[k] = { [0]=1, [1]=2 };
    //     inline constexpr auto a = arr_of( raw );   // arr<u64, k>

    template<typename Type, usize N>
    [[nodiscard]] constexpr auto arr_of( Type (&& source)[N] ) noexcept
        -> arr<Type, N>
    {
        return arr<Type, N>{ std::to_array( std::move( source ) ) };
    }

    template<typename Type, usize N>
    [[nodiscard]] constexpr auto arr_of( Type const ( &source )[N] ) noexcept
        -> arr<Type, N>
    {
        return arr<Type, N>{ std::to_array( source ) };
    }

    // From a fixed byte block (std::array<u8, N> / ct::byte_block) -- e.g. the
    // result of a `"_vstr"` literal or `ct::vstr<"..">` -- into a typed stx::arr:
    //     "EMOJIDAT"_vstr           -> byte_block<8> (std::array<u8, 8>)
    //     arr_of("EMOJIDAT"_vstr)   -> arr<u8, 8>
    //     arr_of<u8>("EMOJIDAT"_vstr) -> arr<u8, 8>
    template<typename Type = u8, usize N>
    [[nodiscard]] constexpr auto arr_of( std::array<u8, N> const& block ) noexcept
        -> arr<Type, N>
    {
        arr<Type, N> out{};
        for ( usize i = 0; i < N; ++i ) out[ i ] = static_cast<Type>( block[ i ] );
        return out;
    }
}
