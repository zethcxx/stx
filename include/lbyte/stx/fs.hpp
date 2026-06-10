#pragma once
#include "./core.hpp"
#include "./mem.hpp"

#include <expected>
#include <istream>
#include <memory>
#include <ostream>
#include <system_error>
#include <utility>
#include <vector>
#include <span>
#include <filesystem>

#if !defined(_WIN32)
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <unistd.h>
#else
    #if !defined(NOMINMAX)
        #define NOMINMAX
    #endif
    #if !defined(WIN32_LEAN_AND_MEAN)
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#endif

namespace lbyte::stx
{
    // ALLOCATOR ----------------------------------------------------------------
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

    // DIRTY VECTOR --------------------------------------------------------------
    template<binary_readable Type = u8>
    using dirty_vector = std::vector<Type, details::vec_init_allocator<Type>>;

    // FILE STREAM UTILITIES ----------------------------------------------------
    namespace fs {

        enum class origin : u8
        {
            begin  ,
            current,
            end    ,
        };

        inline void setposfs(
            std::istream& file,
            const off_s offset,
            const origin dir = origin::begin
        ) noexcept {
            file.seekg(
                scast<std::streamoff>( offset.get() ),
                static_cast<std::ios_base::seekdir>( dir )
            );
        }

        template<binary_readable Type> [[nodiscard]]
        std::expected<Type, std::errc> readfs(
            std::istream& file,
            const off_s offset = off_s{0},
            const origin dir = origin::begin
        ) noexcept {
            Type value;
            setposfs(file, offset, dir);
            file.read(rcast<char*>(&value), sizeof(Type));
            if (file.fail()) [[unlikely]]
                return std::unexpected(std::errc::io_error);
            return value;
        }


        template<binary_readable Type>
        std::expected<void, std::errc> readfs(
            std::istream& file,
            std::span<std::type_identity_t<Type>> out_buffer,
            const off_s offset,
            const origin dir = origin::begin
        ) noexcept {
            setposfs(file, offset, dir);
            file.read(
                rcast<char*>(std::data(out_buffer)),
                scast<std::streamsize>(sizeof(Type) * out_buffer.size())
            );
            if (file.fail()) [[unlikely]]
                return std::unexpected(std::errc::io_error);
            return {};
        }


        template<binary_readable Type = u8> [[nodiscard]]
        std::expected<dirty_vector<Type>, std::errc> readfs(
            std::istream&  file  ,
            const off_s offset,
            const usize    count ,
            const origin   dir   = origin::begin
        ) {
            dirty_vector<Type> vec(count);
            auto result = readfs<Type>(file, std::span<std::type_identity_t<Type>>{vec}, offset, dir);
            if (!result) [[unlikely]]
                return std::unexpected(result.error());
            return vec;
        }


        template<binary_readable Type, usize Size >
        requires ( Size > 0 ) [[nodiscard]]
        std::expected<std::array<Type, Size>, std::errc> readfs(
            std::istream& file  ,
            const off_s offset = off_s{0},
            const origin dir = origin::begin
        ) noexcept {
            std::array<Type, Size> arr;
            auto result = readfs<Type>(file, std::span<std::type_identity_t<Type>>{arr}, offset, dir);
            if (!result) [[unlikely]]
                return std::unexpected(result.error());
            return arr;
        }

        inline
        void skipfs(
            std::istream& file,
            const off_s offset
        ) noexcept {
            setposfs(file, offset, origin::current);
        }

        // FILE WRITE UTILITIES ----------------------------------------------------
        inline void setposos(
            std::ostream& file,
            const off_s offset,
            const origin dir = origin::begin
        ) noexcept {
            file.seekp(
                scast<std::streamoff>( offset.get() ),
                static_cast<std::ios_base::seekdir>( dir )
            );
        }

        template<binary_readable Type>
        std::expected<void, std::errc> writefs(
            std::ostream& file,
            const off_s offset,
            const Type& value
        ) noexcept {
            setposos(file, offset);
            file.write(
                rcast<const char*>(&value),
                sizeof(Type)
            );
            if (file.fail()) [[unlikely]]
                return std::unexpected(std::errc::io_error);
            return {};
        }

        template<binary_readable Type>
        std::expected<void, std::errc> writefs(
            std::ostream& file,
            const off_s offset,
            std::span<const Type> buffer
        ) noexcept {
            setposos(file, offset);
            file.write(
                rcast<const char*>(std::data(buffer)),
                scast<std::streamsize>(sizeof(Type) * buffer.size())
            );
            if (file.fail()) [[unlikely]]
                return std::unexpected(std::errc::io_error);
            return {};
        }

        void skipos(
            std::ostream& file,
            const off_s offset
        ) noexcept {
            setposos(file, offset, origin::current);
        }

    } // namespace fs

    using fs::origin;

    // MAP_FILE -------------------------------------------------------------------

    enum class map_flag : u8 {
        none     = 0,
        write    = 1 << 0,
        exec     = 1 << 1,
        shared   = 1 << 2,
        priv     = 1 << 3,
        populate = 1 << 4,
    };

    constexpr map_flag operator|(map_flag a, map_flag b) noexcept {
        return static_cast<map_flag>(static_cast<u8>(a) | static_cast<u8>(b));
    }

    constexpr bool operator&(map_flag a, map_flag b) noexcept {
        return (static_cast<u8>(a) & static_cast<u8>(b)) != 0;
    }

    // --- memcur<ByteType> (bounded cursor over a buffer) -----------------------

    template<buffer_type ByteType = std::byte>
    class memcur
    {
    protected:
        ptr<ByteType>  base_;
        usize          size_{};
        ptr<ByteType>  cur_;

    public:
        memcur() noexcept = default;

        memcur(memcur&& other) noexcept
            : base_(std::exchange(other.base_, ptr<ByteType>{}))
            , size_(std::exchange(other.size_, 0))
            , cur_(std::exchange(other.cur_, ptr<ByteType>{}))
        {}

        constexpr memcur& operator=(memcur&& other) noexcept
        {
            if (this != &other) {
                base_ = std::exchange(other.base_, ptr<ByteType>{});
                size_ = std::exchange(other.size_, 0);
                cur_  = std::exchange(other.cur_, ptr<ByteType>{});
            }
            return *this;
        }

        memcur(std::span<ByteType> buf) noexcept
            : base_(buf.data())
            , size_(buf.size())
            , cur_(base_)
        {}

        memcur(ptr<ByteType> base, usize size) noexcept
            : base_(base)
            , size_(size)
            , cur_(base_)
        {}

        template<typename Ptr>
            requires std::is_pointer_v<Ptr>
                  && std::convertible_to<Ptr, const void*>
        [[nodiscard]] constexpr memcur(Ptr data, usize size) noexcept
            : base_(data)
            , size_(size)
            , cur_(base_)
        {}

    protected:
        // for inheritance: construct with cursor already positioned
        memcur(ptr<ByteType> base, usize size, off_s pos) noexcept
            : base_(base)
            , size_(size)
            , cur_(base + off_s{scast<off_s::value_type>(pos.get() < 0 ? 0 : pos.get())})
        {
            if (cur_.addr() < base_.addr()) cur_ = base_;
            if (cur_.addr() > base_.addr() + size_) cur_ = base_ + off_s{scast<off_s::value_type>(size_)};
        }

    public:
        // --- state ---------------------------------------------------------

        explicit operator bool() const noexcept { return base_.addr() != 0; }
        usize size() const noexcept { return size_; }
        uptr  base() const noexcept { return base_.addr(); }

        [[nodiscard]] std::span<const std::byte> bytes() const noexcept {
            return { rcast<const std::byte*>(base_.addr()), size_ };
        }

        [[nodiscard]] std::span<std::byte> bytes() noexcept
            requires (!std::is_const_v<ByteType>)
        {
            return { rcast<std::byte*>(base_.addr()), size_ };
        }

        template<typename T = ByteType>
        [[nodiscard]] constexpr ptr<T> as_p() const noexcept {
            return ptr<T>(base_.addr());
        }

        // --- cursor --------------------------------------------------------

        void seek(off_s off, origin dir = origin::begin) noexcept {
            auto const o = off.get();
            switch (dir) {
                case origin::begin:   cur_ = base_ + off_s{o < 0 ? off_s::value_type{0} : o}; break;
                case origin::current: cur_ = cur_ + off; break;
                case origin::end:     cur_ = base_ + off_s{scast<off_s::value_type>(size_) + o}; break;
            }
            if (cur_.addr() < base_.addr()) cur_ = base_;
            if (cur_.addr() > base_.addr() + size_) cur_ = base_ + off_s{scast<off_s::value_type>(size_)};
        }

        void skip(const off_s offset) noexcept { seek(offset, origin::current); }

        off_s tell() const noexcept { return cur_.diff(base_); }
        off_s remaining() const noexcept {
            auto rem = scast<off_s::value_type>(size_) - tell().get();
            return off_s{rem < 0 ? off_s::value_type{0} : rem};
        }

        // --- pop (read + advance) ------------------------------------------

        template<binary_readable T>
        T pop() noexcept {
            return cur_.template pop<T>();
        }

        template<bounded_array U>
        details::bounded_array_t<U> pop() noexcept {
            return cur_.template pop<U>();
        }

        // --- push (write + advance) ----------------------------------------

        template<binary_readable T>
            requires (!contiguous_buffer<T>) && (!std::is_const_v<ByteType>)
        auto& push(const T& value) noexcept {
            cur_.push(value);
            return *this;
        }

        template<contiguous_buffer R>
            requires (!std::is_const_v<ByteType>)
        auto& push(R&& buf) noexcept {
            cur_.push(std::forward<R>(buf));
            return *this;
        }

        // --- zero-copy view (no advance) -----------------------------------

        template<bounded_array U>
        std::span<const std::remove_all_extents_t<U>> as_view() noexcept
        {
            using element_type = std::remove_all_extents_t<U>;
            using flat_array = details::bounded_array_t<U>;
            return std::span<const element_type>(
                rcast<const element_type*>(cur_.addr()),
                sizeof(flat_array) / sizeof(element_type)
            );
        }

        template<binary_readable T>
        std::span<const T> as_view(usize count) noexcept
        {
            return std::span<const T>( rcast<const T*>( cur_.addr() ), count );
        }

        // --- read into (no advance) / pop into (advance) -------------------

        template<writable_buffer R>
        void read_into(R&& buf) noexcept
        {
            auto const bytes = std::size(buf) * sizeof(*std::data(buf));
            std::memcpy(
                rcast<std::byte*>(std::data(buf)),
                rcast<const std::byte*>(cur_.addr()),
                static_cast<usize>(bytes)
            );
        }

        template<writable_buffer R>
        auto& pop_into(R&& buf) noexcept
        {
            auto const bytes = std::size(buf) * sizeof(*std::data(buf));
            std::memcpy(
                rcast<std::byte*>(std::data(buf)),
                rcast<const std::byte*>(cur_.addr()),
                static_cast<usize>(bytes)
            );
            cur_ += off_s{scast<off_s::value_type>(bytes)};
            return *this;
        }

        std::string_view read_strvw() noexcept
        {
            return read_strvw(size_ - static_cast<usize>(tell().get()));
        }

        std::string_view read_strvw(usize max) noexcept
        {
            auto pos = static_cast<usize>(tell().get());
            auto rem = size_ - pos;
            auto avail = std::min(max, rem);
            if (avail == 0) return {};
            auto* base = rcast<const char*>(cur_.addr());
            auto len = strnlen(base, avail);
            cur_ = cur_ + off_s{scast<off_s::value_type>(len < avail ? len + 1 : avail)};
            return {base, len};
        }
    };

    // --- deduction guides for memcur -----------------------------------------

    template<lbyte::stx::buffer_type B>
    memcur(B*, usize) -> memcur<B>;

    template<typename T>
        requires (!lbyte::stx::buffer_type<T>)
    memcur(T*, usize) -> memcur<std::conditional_t<std::is_const_v<T>, const std::byte, std::byte>>;

    class map_file : private memcur<std::byte>
    {
        void*           raw_    = nullptr;  // mmap base (for unmap)
        usize           raw_sz_ = 0;        // mapped size (for munmap)
        map_flag        flags_{};

        auto unmap() noexcept -> void;

        struct sys_ctor_tag {};
        map_file(sys_ctor_tag, void* raw, usize raw_sz, ptr<std::byte> data, usize sz, off_s pos, map_flag fl) noexcept
            : memcur(data, sz, pos)
            , raw_(raw), raw_sz_(raw_sz), flags_(fl)
        {}

        static auto sys_map(const std::filesystem::path&, usize offset, usize size, map_flag) noexcept
            -> std::expected<map_file, std::errc>;

    public:
        map_file() noexcept = default;

        map_file(map_file&& other) noexcept
            : memcur(std::move(other))
            , raw_(std::exchange(other.raw_, nullptr))
            , raw_sz_(std::exchange(other.raw_sz_, 0))
            , flags_(std::exchange(other.flags_, map_flag::none))
        {}

        auto operator=(map_file&& other) noexcept -> map_file&
        {
            if (this != &other) {
                unmap();
                memcur::operator=(std::move(other));
                raw_    = std::exchange(other.raw_, nullptr);
                raw_sz_ = std::exchange(other.raw_sz_, 0);
                flags_  = std::exchange(other.flags_, map_flag::none);
            }
            return *this;
        }

        map_file(const map_file&) = delete;
        auto operator=(const map_file&) -> map_file& = delete;

        ~map_file() noexcept { unmap(); }

        // --- factory -------------------------------------------------------

        static auto open(const std::filesystem::path& path, map_flag flags = {}) noexcept
            -> std::expected<map_file, std::errc>
        {
            return sys_map(path, 0, 0, flags);
        }

        static auto open(const std::filesystem::path& path, off_s offset, usize size, map_flag flags = {}) noexcept
            -> std::expected<map_file, std::errc>
        {
            if (offset.get() < 0)
                return std::unexpected(std::errc::invalid_argument);
            return sys_map(path, static_cast<usize>(offset.get()), size, flags);
        }

        // --- re-exports from memcur ---------------------------------------

        using memcur::operator bool;
        using memcur::size;
        using memcur::base;
        using memcur::seek;
        using memcur::skip;
        using memcur::tell;
        using memcur::remaining;
        using memcur::pop;
        using memcur::push;
        using memcur::as_view;
        using memcur::read_into;
        using memcur::pop_into;
        using memcur::read_strvw;
        using memcur::bytes;
        using memcur::as_p;

        // --- map_file-specific --------------------------------------------

        map_flag flags() const noexcept { return flags_; }
        bool     is_alive() const noexcept { return memcur::operator bool(); }

        void swap(map_file& other) noexcept
        {
            using std::swap;
            swap(static_cast<memcur<std::byte>&>(*this), static_cast<memcur<std::byte>&>(other));
            swap(raw_, other.raw_);
            swap(raw_sz_, other.raw_sz_);
            swap(flags_, other.flags_);
        }
    };

    // --- readfs/writefs overloads for map_file -------------------------------

    namespace fs {

        template<binary_readable Type> [[nodiscard]]
        std::expected<Type, std::errc> readfs(
            const map_file& m,
            const off_s offset
        ) noexcept
        {
            if (offset.get() + static_cast<off_s::value_type>(sizeof(Type)) > static_cast<off_s::value_type>(m.size()))
                return std::unexpected(std::errc::argument_out_of_domain);
            return m.as_p<Type>()[offset].template read<Type>();
        }

        template<binary_readable Type>
        std::expected<void, std::errc> writefs(
            map_file& m,
            const off_s offset,
            const Type& value
        ) noexcept
        {
            if (!(m.flags() & map_flag::write))
                return std::unexpected(std::errc::permission_denied);
            if (offset.get() + static_cast<off_s::value_type>(sizeof(Type)) > static_cast<off_s::value_type>(m.size()))
                return std::unexpected(std::errc::argument_out_of_domain);
            m.as_p<Type>()[offset].write(value);
            return {};
        }

        // --- readfs/writefs overloads for spans (positional, no reader_view needed) -

        template<binary_readable Type> [[nodiscard]]
        std::expected<Type, std::errc> readfs(
            std::span<const std::byte> buf,
            const off_s offset
        ) noexcept
        {
            auto end = offset.get() + static_cast<off_s::value_type>(sizeof(Type));
            if (end > static_cast<off_s::value_type>(buf.size()))
                return std::unexpected(std::errc::argument_out_of_domain);
            Type val;
            std::memcpy(&val, buf.data() + offset.get(), sizeof(Type));
            return val;
        }

        template<binary_readable Type>
        std::expected<void, std::errc> writefs(
            std::span<std::byte> buf,
            const off_s offset,
            const Type& value
        ) noexcept
        {
            auto end = offset.get() + static_cast<off_s::value_type>(sizeof(Type));
            if (end > static_cast<off_s::value_type>(buf.size()))
                return std::unexpected(std::errc::argument_out_of_domain);
            std::memcpy(buf.data() + offset.get(), &value, sizeof(Type));
            return {};
        }

    } // namespace fs

    // --- platform ------------------------------------------------------------

    #if defined(_WIN32)

        inline auto map_file::sys_map(
            const std::filesystem::path& path,
            usize offset,
            usize size,
            map_flag flags
        ) noexcept -> std::expected<map_file, std::errc>
        {
            DWORD dwDesiredAccess = GENERIC_READ;
            DWORD flProtect = PAGE_READONLY;
            DWORD dwFileMapAccess = FILE_MAP_READ;

            if (flags & map_flag::write) {
                dwDesiredAccess |= GENERIC_WRITE;
                if (flags & map_flag::priv) {
                    flProtect = PAGE_WRITECOPY;
                    dwFileMapAccess = FILE_MAP_COPY;
                } else {
                    flProtect = PAGE_READWRITE;
                    dwFileMapAccess = FILE_MAP_ALL_ACCESS;
                }
            }
            if (flags & map_flag::exec) {
                if (flags & map_flag::write) {
                    if (flags & map_flag::priv) {
                        flProtect = PAGE_EXECUTE_WRITECOPY;
                    } else {
                        flProtect = PAGE_EXECUTE_READWRITE;
                    }
                } else {
                    flProtect = PAGE_EXECUTE_READ;
                }
                dwFileMapAccess = FILE_MAP_EXECUTE | (dwFileMapAccess & ~FILE_MAP_COPY);
                if (flags & map_flag::priv) dwFileMapAccess |= FILE_MAP_COPY;
            }
            if (flags & map_flag::shared) {
                if (flags & map_flag::priv)
                    return std::unexpected(std::errc::invalid_argument);
            }
            if (flags & map_flag::priv) {
                dwDesiredAccess |= GENERIC_WRITE;
            }

            HANDLE hFile = CreateFileW(
                path.c_str(),
                dwDesiredAccess,
                FILE_SHARE_READ,
                nullptr,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                nullptr
            );
            if (hFile == INVALID_HANDLE_VALUE)
                return std::unexpected(std::errc::no_such_file_or_directory);

            LARGE_INTEGER fileSize;
            if (!GetFileSizeEx(hFile, &fileSize)) {
                CloseHandle(hFile);
                return std::unexpected(std::errc::io_error);
            }

            usize file_size = static_cast<usize>(fileSize.QuadPart);
            if (offset > file_size) {
                CloseHandle(hFile);
                return std::unexpected(std::errc::argument_out_of_domain);
            }
            if (size == 0) size = file_size - offset;
            if (size > file_size - offset) {
                CloseHandle(hFile);
                return std::unexpected(std::errc::argument_out_of_domain);
            }

            // MapViewOfFile requires offset to be aligned to allocation granularity
            SYSTEM_INFO si;
            GetSystemInfo(&si);
            usize gran = static_cast<usize>(si.dwAllocationGranularity);
            usize align_off = (offset / gran) * gran;
            usize delta = offset - align_off;
            usize view_sz = size + delta;

            DWORD dwFileOffsetLow  = static_cast<DWORD>(align_off & 0xFFFFFFFF);
            DWORD dwFileOffsetHigh = static_cast<DWORD>((align_off >> 32) & 0xFFFFFFFF);

            HANDLE hMap = CreateFileMappingW(
                hFile,
                nullptr,
                flProtect,
                0, 0,
                nullptr
            );
            if (!hMap) {
                CloseHandle(hFile);
                return std::unexpected(std::errc::io_error);
            }

            void* raw = MapViewOfFile(
                hMap,
                dwFileMapAccess,
                dwFileOffsetHigh,
                dwFileOffsetLow,
                view_sz
            );

            CloseHandle(hMap);
            CloseHandle(hFile);

            if (!raw)
                return std::unexpected(std::errc::io_error);

            auto* data = static_cast<std::byte*>(raw) + delta;
            return map_file(sys_ctor_tag{}, raw, view_sz, data, size, off_s{0}, flags);
        }

        inline auto map_file::unmap() noexcept -> void
        {
            if (raw_) UnmapViewOfFile(raw_);
        }

    #else // Linux / POSIX

        inline auto map_file::sys_map(
            const std::filesystem::path& path,
            usize offset,
            usize size,
            map_flag flags
        ) noexcept -> std::expected<map_file, std::errc>
        {
            if ((flags & map_flag::shared) && (flags & map_flag::priv))
                return std::unexpected(std::errc::invalid_argument);

            int oflags = O_RDONLY;
            int prot   = PROT_READ;
            int mflags = MAP_PRIVATE;

            if (flags & map_flag::write) {
                oflags = O_RDWR;
                prot |= PROT_WRITE;
                mflags = MAP_SHARED;
            }
            if (flags & map_flag::exec) {
                prot |= PROT_EXEC;
            }
            if (flags & map_flag::shared) {
                mflags = MAP_SHARED;
            }
            if (flags & map_flag::priv) {
                mflags = MAP_PRIVATE;
            }
            if (flags & map_flag::populate) {
                mflags |= MAP_POPULATE;
            }

            int fd = ::open(path.c_str(), oflags);
            if (fd < 0)
                return std::unexpected(std::errc::no_such_file_or_directory);

            struct stat st;
            if (::fstat(fd, &st) < 0) {
                ::close(fd);
                return std::unexpected(std::errc::io_error);
            }

            usize file_size = static_cast<usize>(st.st_size);
            if (offset > file_size) {
                ::close(fd);
                return std::unexpected(std::errc::argument_out_of_domain);
            }
            if (size == 0) size = file_size - offset;
            if (size > file_size - offset) {
                ::close(fd);
                return std::unexpected(std::errc::argument_out_of_domain);
            }

            // mmap requires page-aligned offset
            long page = sysconf(_SC_PAGE_SIZE);
            if (page <= 0) page = 4096;
            usize align_off = (offset / static_cast<usize>(page)) * static_cast<usize>(page);
            usize delta = offset - align_off;
            usize map_sz = size + delta;

            void* raw = ::mmap(
                nullptr,
                map_sz,
                prot,
                mflags,
                fd,
                static_cast<off_t>(align_off)
            );

            ::close(fd);

            if (raw == MAP_FAILED)
                return std::unexpected(std::errc::io_error);

            auto* data = static_cast<std::byte*>(raw) + delta;
            return map_file(sys_ctor_tag{}, raw, map_sz, data, size, off_s{0}, flags);
        }

        inline auto map_file::unmap() noexcept -> void
        {
            if (raw_) ::munmap(raw_, raw_sz_);
        }

    #endif

}

