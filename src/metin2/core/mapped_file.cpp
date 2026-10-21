#include "mapped_file.h"

#include <metin2/log/log.h>

#include <algorithm>
#include <cerrno>
#include <cstring>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace metin2::inline core {

    mapped_file::mapped_file() = default;

    mapped_file::~mapped_file() {
        destroy();
    }

    auto mapped_file::create(const char *filename) -> bool {
        destroy();

        filename_ = filename;
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

    void mapped_file::destroy() {
        unmap();

        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }

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

        if (fd_ < 0) {
            metin2::log::error("Cannot map file {}: it is not open", filename_.c_str());
            return 0;
        }

        const auto page_size = static_cast<size_t>(sysconf(_SC_PAGESIZE));
        const size_t start = (data_offset_ / page_size) * page_size;
        const size_t view_delta = data_offset_ - start;
        length_ = view_delta + map_size_;

        if (length_ == 0) {
            // mmap rejects zero-length mappings; an empty view has nothing to point at.
            data_value_ = nullptr;
            *dest = nullptr;
            seek_position_ = 0;
            link(0, nullptr);
            return 0;
        }

        map_data_ = mmap(nullptr, length_, PROT_READ, MAP_PRIVATE, fd_, static_cast<off_t>(start));
        if (map_data_ == MAP_FAILED) {
            map_data_ = nullptr;
            metin2::log::error("Cannot map file {}: {}", filename_.c_str(), strerror(errno));
            return 0;
        }

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
        if (map_data_) {
            if (munmap(map_data_, length_) != 0)
                metin2::log::error("Cannot unmap file: {}", strerror(errno));
            map_data_ = nullptr;
        }
        length_ = 0;
        data_value_ = nullptr;
    }
}
