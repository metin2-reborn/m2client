#include "mapped_file.h"

#include <metin2/log/log.h>

#include <algorithm>
#include <cerrno>
#include <cstring>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace metin2::inline core {

    namespace {
        // Mappings must start at a multiple of this (the allocation granularity on Windows, the page
        // size elsewhere).
        auto mapping_granularity() -> size_t {
#ifdef _WIN32
            SYSTEM_INFO info;
            GetSystemInfo(&info);
            return info.dwAllocationGranularity;
#else
            return static_cast<size_t>(sysconf(_SC_PAGESIZE));
#endif
        }

#ifdef _WIN32
        auto last_error_text() -> std::string {
            char *text = nullptr;
            const DWORD length = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, GetLastError(), 0, reinterpret_cast<char *>(&text), 0, nullptr);
            std::string result = length && text ? std::string(text, length) : std::string("unknown error");
            if (text)
                LocalFree(text);
            while (!result.empty() && (result.back() == '\n' || result.back() == '\r'))
                result.pop_back();
            return result;
        }
#endif
    }

    mapped_file::mapped_file() = default;

    mapped_file::~mapped_file() {
        destroy();
    }

    auto mapped_file::is_open() const -> bool {
#ifdef _WIN32
        return file_ != nullptr;
#else
        return fd_ >= 0;
#endif
    }

    auto mapped_file::create(const char *filename) -> bool {
        destroy();

        filename_ = filename;
#ifdef _WIN32
        HANDLE file = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE)
            return false;

        LARGE_INTEGER file_size;
        if (!GetFileSizeEx(file, &file_size)) {
            CloseHandle(file);
            return false;
        }
        file_ = file;
        file_base::size_ = static_cast<uint32_t>(file_size.QuadPart);
#else
        fd_ = open(filename, O_RDONLY | O_CLOEXEC);
        if (fd_ < 0)
            return false;

        struct stat st;
        if (fstat(fd_, &st) != 0) {
            ::close(fd_);
            fd_ = -1;
            return false;
        }

        file_base::size_ = static_cast<uint32_t>(st.st_size);
#endif
        mode_value_ = FILEMODE_READ;
        return true;
    }

    auto mapped_file::create(const char *filename, const void **dest, int offset, int size) -> bool {
        if (!mapped_file::create(filename))
            return false;

        return map(dest, offset, size) > 0;
    }

    auto mapped_file::get() -> const void * {
        return data_value_;
    }

    void mapped_file::link(uint32_t buf_size, const void *buf_data) {
        buf_link_size_ = buf_size;
        buf_link_data_ = static_cast<uint8_t *>(const_cast<void *>(buf_data));
    }

    auto mapped_file::adopt(std::vector<uint8_t> &&buffer_value) -> const void * {
        owned_data_ = std::move(buffer_value);
        seek_position_ = 0;
        link(static_cast<uint32_t>(owned_data_.size()), owned_data_.data());
        return owned_data_.data();
    }

    void mapped_file::close_file() {
#ifdef _WIN32
        if (file_) {
            CloseHandle(static_cast<HANDLE>(file_));
            file_ = nullptr;
        }
#else
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
#endif
    }

    void mapped_file::destroy() {
        unmap();
        close_file();

        owned_data_.clear();
        owned_data_.shrink_to_fit();

        buf_link_data_ = nullptr;
        buf_link_size_ = 0;

        seek_position_ = 0;
        data_offset_ = 0;
        map_size_ = 0;

        file_base::destroy();
    }

    auto mapped_file::seek(uint32_t offset, int seek_type_value) -> int {
        switch (seek_type_value) {
        case SEEK_TYPE_BEGIN:
            seek_position_ = std::min(offset, size()); // size(): linked buffers have no file size
            break;

        case SEEK_TYPE_CURRENT:
            seek_position_ = std::min(seek_position_ + offset, size());
            break;

        case SEEK_TYPE_END:
            seek_position_ = offset > size() ? 0 : size() - offset;
            break;
        }

        return seek_position_;
    }

    auto mapped_file::map(const void **dest, int offset, int size) -> int {
        data_offset_ = offset;
        map_size_ = size == 0 ? file_base::size_ : size;

        if (data_offset_ + map_size_ > file_base::size_)
            return 0;

        if (!is_open()) {
            metin2::log::error("Cannot map file {}: it is not open", filename_.c_str());
            return 0;
        }

        const size_t granularity = mapping_granularity();
        const size_t start = (data_offset_ / granularity) * granularity;
        const size_t view_delta = data_offset_ - start;
        length_ = view_delta + map_size_;

        if (length_ == 0) {
            // A zero-length mapping is rejected; an empty view has nothing to point at.
            data_value_ = nullptr;
            *dest = nullptr;
            seek_position_ = 0;
            link(0, nullptr);
            return 0;
        }

#ifdef _WIN32
        HANDLE mapping = CreateFileMappingA(static_cast<HANDLE>(file_), nullptr, PAGE_READONLY, 0, 0, nullptr);
        if (!mapping) {
            metin2::log::error("Cannot map file {}: {}", filename_.c_str(), last_error_text());
            return 0;
        }
        const auto start64 = static_cast<unsigned long long>(start);
        void *view = MapViewOfFile(mapping, FILE_MAP_READ, static_cast<DWORD>(start64 >> 32), static_cast<DWORD>(start64 & 0xffffffffu), length_);
        if (!view) {
            metin2::log::error("Cannot map file {}: {}", filename_.c_str(), last_error_text());
            CloseHandle(mapping);
            return 0;
        }
        mapping_ = mapping;
        map_data_ = view;
#else
        map_data_ = mmap(nullptr, length_, PROT_READ, MAP_PRIVATE, fd_, static_cast<off_t>(start));
        if (map_data_ == MAP_FAILED) {
            map_data_ = nullptr;
            metin2::log::error("Cannot map file {}: {}", filename_.c_str(), strerror(errno));
            return 0;
        }
#endif

        data_value_ = static_cast<char *>(map_data_) + view_delta;
        *dest = data_value_;
        seek_position_ = 0;

        link(map_size_, data_value_);

        return map_size_;
    }

    auto mapped_file::current_seek_point() -> uint8_t * {
        return buf_link_data_ + seek_position_;
    }

    auto mapped_file::size() -> uint32_t {
        return buf_link_size_;
    }

    auto mapped_file::get_position() -> uint32_t {
        return data_offset_;
    }

    auto mapped_file::read(void *dest, int bytes) -> bool {
        if (seek_position_ + bytes > size())
            return false;

        memcpy(dest, current_seek_point(), bytes);
        seek_position_ += bytes;
        return true;
    }

    void mapped_file::unmap() {
#ifdef _WIN32
        if (map_data_) {
            UnmapViewOfFile(map_data_);
            map_data_ = nullptr;
        }
        if (mapping_) {
            CloseHandle(static_cast<HANDLE>(mapping_));
            mapping_ = nullptr;
        }
#else
        if (map_data_) {
            if (munmap(map_data_, length_) != 0)
                metin2::log::error("Cannot unmap file: {}", strerror(errno));
            map_data_ = nullptr;
        }
#endif
        length_ = 0;
        data_value_ = nullptr;
    }
}
