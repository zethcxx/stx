#pragma once
#include "./core.hpp"
#include "./mem.hpp"

#include <istream>
#include <utility>
#include <vector>

namespace stx
{
    namespace details
    {
        template<typename T, typename Allocator = std::allocator<T>>
        class vec_init_allocator : public Allocator
        {
            using traits = std::allocator_traits<Allocator>;
        public:
            template<typename U> struct rebind { using other = vec_init_allocator<U, typename traits::template rebind_alloc<U>>; };

            using Allocator::Allocator;

            template<typename U>
            void construct(U* ptr) noexcept(std::is_nothrow_default_constructible_v<U>) {
                if constexpr (std::is_default_constructible_v<U>) {
                    ::new(static_cast<void*>(ptr)) U;
                }
            }

            template<typename U, typename... Args>
            void construct(U* ptr, Args&&... args) {
                traits::construct(static_cast<Allocator&>(*this), ptr, std::forward<Args>(args)...);
            }
        };
    }

    template<binary_readable Type = u8>
    using dirty_vector = std::vector<Type, details::vec_init_allocator<Type>>;

    inline void setposfs(
        std::istream& file,
        const offset_t offset,
        const origin dir = origin::begin
    ) {
        file.seekg(
            scast<std::streamoff>( offset.get() ),
            static_cast<std::ios_base::seekdir>( dir )
        );
    }

    template<binary_readable Type> [[nodiscard]]
    Type readfs(
        std::istream& file,
        const offset_t offset = offset_t {},
        const origin dir = origin::begin
    ) {
        Type value;

        setposfs( file, offset, dir);
        file.read (
            rcast<char*>( &value ),
            sizeof( Type )
        );

        return value;
    }


    template<binary_readable Type> inline
    void readfs(
        std::istream& file,
        std::span<Type> out_buffer
    ) {
        file.read(
            rcast<char*>( out_buffer ),
            scast<std::streamsize>( sizeof( Type ) * out_buffer.size() )
        );
    }


    template<binary_readable Type = u8> [[nodiscard]]
    dirty_vector<Type> readfs(
        std::istream&  file  ,
        const offset_t offset,
        const usize    count ,
        const origin   dir   = origin::begin
    ) {
        dirty_vector<Type> vec( count );

        setposfs( file, offset    , dir   );
        readfs  ( file, vec.data(), count );

        return vec;
    }


    template<binary_readable Type, usize Size >
    requires ( Size > 0 ) [[nodiscard]]
    std::array<Type, Size> readfs(
        std::istream& file  ,
        const offset_t offset,
        const origin dir = origin::begin
    ) {
        std::array< Type, Size > arr;

        setposfs( file, offset, dir );
        file.read (
            rcast<char*>( arr.data()),
            scast<std::streamsize>( sizeof( Type ) * Size )
        );

        return arr;
    }

    template<binary_readable Type = std::byte> inline
    void skipfs(
        std::istream& file,
        const offset_t offset
    ) {
        setposfs( file, offset, origin::current );
    }

    inline bool last_read_ok( const std::istream& file ) noexcept {
        return file.good() and not file.eof();
    }
}
