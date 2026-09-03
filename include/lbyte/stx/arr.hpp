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
            else // byte_offset newtype
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

        base_array elems{};

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

    // ---- arr_of factory (std::to_array style) --------------------------------
    // Deduces Type and N from a (brace) array; `arr_of<off_t>({...})` and
    // `arr_of({...})` both work.

    template<typename Type, usize N>
    [[nodiscard]] constexpr auto arr_of( Type (&& source)[N] ) noexcept
        -> arr<Type, N>
    {
        return arr<Type, N>{ std::to_array( std::move( source ) ) };
    }
}
