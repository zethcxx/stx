#pragma once
#include "./core.hpp"
#include "./mem.hpp"

#include <expected>
#include <istream>
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

    class map_file
    {
        void*  raw_    = nullptr;  // mmap/CreateFileMapping base (for unmap)
        usize  raw_sz_ = 0;        // mapped size (for munmap)
        void*  data_   = nullptr;  // effective data start
        usize  size_   = 0;        // logical file region size
        usize  pos_    = 0;
        map_flag flags_{};

        auto unmap() noexcept -> void;

        struct sys_ctor_tag {};
        map_file(sys_ctor_tag, void* raw, usize raw_sz, void* data, usize sz, usize pos, map_flag fl) noexcept
            : raw_(raw), raw_sz_(raw_sz), data_(data), size_(sz), pos_(pos), flags_(fl) {}

        static auto sys_map(const std::filesystem::path&, usize offset, usize size, map_flag) noexcept
            -> std::expected<map_file, std::errc>;

    public:
        map_file() noexcept = default;

        map_file(map_file&& other) noexcept
            : raw_(std::exchange(other.raw_, nullptr))
            , raw_sz_(std::exchange(other.raw_sz_, 0))
            , data_(std::exchange(other.data_, nullptr))
            , size_(std::exchange(other.size_, 0))
            , pos_(std::exchange(other.pos_, 0))
            , flags_(std::exchange(other.flags_, map_flag::none))
        {}

        auto operator=(map_file&& other) noexcept -> map_file&
        {
            if (this != &other) {
                unmap();
                raw_    = std::exchange(other.raw_, nullptr);
                raw_sz_ = std::exchange(other.raw_sz_, 0);
                data_   = std::exchange(other.data_, nullptr);
                size_   = std::exchange(other.size_, 0);
                pos_    = std::exchange(other.pos_, 0);
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

        // --- state ---------------------------------------------------------

        [[nodiscard]] explicit operator bool() const noexcept { return data_ != nullptr; }
        [[nodiscard]] usize  size()  const noexcept { return size_; }
        [[nodiscard]] uptr   base()  const noexcept { return reinterpret_cast<uptr>(data_); }
        [[nodiscard]] map_flag flags() const noexcept { return flags_; }

        // --- sequential I/O ------------------------------------------------

        void seek(off_s off, origin dir = origin::begin) noexcept
        {
            auto const o = off.get();
            switch (dir) {
                case origin::begin:
                    pos_ = o > 0 ? static_cast<usize>(o) : usize{0};
                    break;
                case origin::current: {
                    auto const p = static_cast<std::ptrdiff_t>(pos_);
                    auto const r = p + o;
                    pos_ = r > 0 ? static_cast<usize>(r) : usize{0};
                    break;
                }
                case origin::end: {
                    auto const s = static_cast<std::ptrdiff_t>(size_);
                    auto const r = s - o;
                    pos_ = r > 0 ? static_cast<usize>(r) : usize{0};
                    break;
                }
            }
            if (pos_ > size_) pos_ = size_;
        }

        void skip(const off_s offset) noexcept {
            seek(offset, origin::current);
        }

        [[nodiscard]] bool is_alive() const noexcept { return data_ != nullptr; }

        [[nodiscard]] off_s tell() const noexcept { return off_s{static_cast<off_s::value_type>(pos_)}; }
        [[nodiscard]] off_s remaining() const noexcept
        {
            auto rem = size_ - pos_;
            return off_s{static_cast<off_s::value_type>(rem)};
        }

        template<binary_readable T>
        auto read() noexcept -> T
        {
            T val = read<T>(tell());
            pos_ += sizeof(T);
            return val;
        }

        template<binary_readable T>
        auto write(const T& value) noexcept -> void
        {
            write(tell(), value);
            pos_ += sizeof(T);
        }

        // --- zero-copy views ------------------------------------------------

        template<binary_readable T>
        [[nodiscard]] auto read_span(usize count) noexcept -> std::span<const T>
        {
            auto rem = size_ - pos_;
            auto max = rem / sizeof(T);
            if (count > max) return {};
            auto* ptr = static_cast<const T*>(
                static_cast<const void*>(
                    static_cast<const std::byte*>(data_) + pos_));
            pos_ += count * sizeof(T);
            return {ptr, count};
        }

        [[nodiscard]] std::string_view read_strvw() noexcept
        {
            return read_strvw(size_ - pos_);
        }

        [[nodiscard]] std::string_view read_strvw(usize max) noexcept
        {
            auto rem = size_ - pos_;
            auto avail = std::min(max, rem);
            if (avail == 0) return {};
            auto* base = static_cast<const char*>(data_) + pos_;
            auto len = strnlen(base, avail);
            pos_ += len < avail ? len + 1 : avail;
            return {base, len};
        }

        // --- random-access I/O ---------------------------------------------

        template<binary_readable T>
        [[nodiscard]] auto read(off_s off) const noexcept -> T
        {
            return lbyte::stx::read<T>(data_, off);
        }

        template<binary_readable T>
        auto write(off_s off, const T& value) const noexcept -> void
        {
            lbyte::stx::write(data_, off, value);
        }

        // --- span / ptr ----------------------------------------------------

        [[nodiscard]] std::span<const std::byte> bytes() const noexcept
        {
            return { static_cast<const std::byte*>(data_), size_ };
        }

        [[nodiscard]] std::span<std::byte> bytes() noexcept
        {
            return { static_cast<std::byte*>(data_), size_ };
        }

        template<typename T = std::byte>
        [[nodiscard]] auto as_p() const noexcept -> ptr<T>
        {
            return ptr<T>(base());
        }

        // --- swap ----------------------------------------------------------

        void swap(map_file& other) noexcept
        {
            std::swap(raw_,    other.raw_);
            std::swap(raw_sz_, other.raw_sz_);
            std::swap(data_,   other.data_);
            std::swap(size_,   other.size_);
            std::swap(pos_,    other.pos_);
            std::swap(flags_,  other.flags_);
        }
    };

    // --- readfs/writefs overloads for map_file -------------------------------

    template<binary_readable Type> [[nodiscard]]
    std::expected<Type, std::errc> readfs(
        const map_file& m,
        const off_s offset
    ) noexcept
    {
        if (offset.get() + static_cast<off_s::value_type>(sizeof(Type)) > static_cast<off_s::value_type>(m.size()))
            return std::unexpected(std::errc::argument_out_of_domain);
        return m.read<Type>(offset);
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
        m.write<Type>(offset, value);
        return {};
    }

    // --- reader_view (bounded, span-based) ------------------------------------

    class reader_view
    {
        std::span<const std::byte> buf_;
        usize pos_ = 0;

    public:
        reader_view() noexcept = default;

        reader_view(std::span<const std::byte> buf) noexcept
            : buf_(buf) {}

        reader_view(const void* data, usize size) noexcept
            : buf_(static_cast<const std::byte*>(data), size) {}

        // --- state ---------------------------------------------------------

        explicit operator bool() const noexcept { return !buf_.empty(); }
        usize size() const noexcept { return buf_.size(); }

        // --- cursor --------------------------------------------------------

        void seek(off_s off, origin dir = origin::begin) noexcept
        {
            auto const o = off.get();
            switch (dir) {
                case origin::begin:
                    pos_ = o > 0 ? static_cast<usize>(o) : usize{0};
                    break;
                case origin::current: {
                    auto const p = static_cast<std::ptrdiff_t>(pos_);
                    auto const r = p + o;
                    pos_ = r > 0 ? static_cast<usize>(r) : usize{0};
                    break;
                }
                case origin::end: {
                    auto const s = static_cast<std::ptrdiff_t>(buf_.size());
                    auto const r = s - o;
                    pos_ = r > 0 ? static_cast<usize>(r) : usize{0};
                    break;
                }
            }
            if (pos_ > buf_.size()) pos_ = buf_.size();
        }

        void skip(const off_s offset) noexcept {
            seek(offset, origin::current);
        }

        off_s tell() const noexcept {
            return off_s{static_cast<off_s::value_type>(pos_)};
        }

        off_s remaining() const noexcept {
            auto rem = buf_.size() - pos_;
            return off_s{static_cast<off_s::value_type>(rem)};
        }

        // --- read ----------------------------------------------------------

        template<binary_readable T>
        T read() noexcept
        {
            T val;
            std::memcpy(&val, buf_.data() + pos_, sizeof(T));
            pos_ += sizeof(T);
            return val;
        }

        // --- zero-copy views -----------------------------------------------

        template<binary_readable T>
        std::span<const T> read_span(usize count) noexcept
        {
            auto rem = buf_.size() - pos_;
            auto max = rem / sizeof(T);
            if (count > max) return {};
            auto* ptr = static_cast<const T*>(
                static_cast<const void*>(buf_.data() + pos_));
            pos_ += count * sizeof(T);
            return {ptr, count};
        }

        std::string_view read_strvw() noexcept
        {
            return read_strvw(buf_.size() - pos_);
        }

        std::string_view read_strvw(usize max) noexcept
        {
            auto rem = buf_.size() - pos_;
            auto avail = std::min(max, rem);
            if (avail == 0) return {};
            auto* base = reinterpret_cast<const char*>(buf_.data() + pos_);
            auto len = strnlen(base, avail);
            pos_ += len < avail ? len + 1 : avail;
            return {base, len};
        }
    };

    // --- reader_raw (unbounded, pointer-based) -------------------------------

    class reader_raw
    {
        const std::byte* ptr_ = nullptr;
        usize pos_ = 0;

    public:
        reader_raw() noexcept = default;

        explicit reader_raw(const void* ptr) noexcept
            : ptr_(static_cast<const std::byte*>(ptr)) {}

        // --- state ---------------------------------------------------------

        explicit operator bool() const noexcept { return ptr_ != nullptr; }

        // --- cursor --------------------------------------------------------

        void seek(off_s off, origin dir = origin::begin) noexcept
        {
            auto const o = off.get();
            switch (dir) {
                case origin::begin:
                    pos_ = o > 0 ? static_cast<usize>(o) : usize{0};
                    break;
                case origin::current: {
                    auto const p = static_cast<std::ptrdiff_t>(pos_);
                    auto const r = p + o;
                    pos_ = r > 0 ? static_cast<usize>(r) : usize{0};
                    break;
                }
                case origin::end:
                    pos_ = o <= 0 ? usize{0} : pos_;
                    break;
            }
        }

        void skip(const off_s offset) noexcept {
            seek(offset, origin::current);
        }

        off_s tell() const noexcept {
            return off_s{static_cast<off_s::value_type>(pos_)};
        }

        // --- read (no bounds) ----------------------------------------------

        template<binary_readable T>
        T read() noexcept
        {
            T val;
            std::memcpy(&val, ptr_ + pos_, sizeof(T));
            pos_ += sizeof(T);
            return val;
        }

        // --- zero-copy (no bounds) -----------------------------------------

        std::string_view read_strvw() noexcept
        {
            auto* base = reinterpret_cast<const char*>(ptr_ + pos_);
            auto len = std::strlen(base);
            pos_ += len + 1;
            return {base, len};
        }
    };

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

            void* data = static_cast<std::byte*>(raw) + delta;
            return map_file(sys_ctor_tag{}, raw, view_sz, data, size, 0, flags);
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

            void* data = static_cast<std::byte*>(raw) + delta;
            return map_file(sys_ctor_tag{}, raw, map_sz, data, size, 0, flags);
        }

        inline auto map_file::unmap() noexcept -> void
        {
            if (raw_) ::munmap(raw_, raw_sz_);
        }

    #endif

}
