#include "zip_archive.h"

#include <metin2/core/mapped_file.h>
#include <metin2/log/log.h>

#include <cstring>
#include <vector>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <zlib.h>

namespace metin2::inline files {

    namespace {
        constexpr std::uint32_t kLocalHeaderSig = 0x04034b50;
        constexpr std::uint32_t kCentralHeaderSig = 0x02014b50;
        constexpr std::uint32_t kEndOfCentralDirSig = 0x06054b50;
        constexpr std::uint32_t kZip64LocatorSig = 0x07064b50;
        constexpr std::uint32_t kZip64EndSig = 0x06064b50;

        constexpr std::uint16_t kMethodStored = 0;
        constexpr std::uint16_t kMethodDeflate = 8;
        constexpr std::uint16_t kFlagEncrypted = 0x0001;

        auto read16(const std::uint8_t *p) -> std::uint16_t {
            return static_cast<std::uint16_t>(p[0] | (p[1] << 8));
        }

        auto read32(const std::uint8_t *p) -> std::uint32_t {
            return static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8) |
                   (static_cast<std::uint32_t>(p[2]) << 16) | (static_cast<std::uint32_t>(p[3]) << 24);
        }

        auto read64(const std::uint8_t *p) -> std::uint64_t {
            return static_cast<std::uint64_t>(read32(p)) | (static_cast<std::uint64_t>(read32(p + 4)) << 32);
        }
    }

    namespace assets {
        auto starts_with_no_case(std::string_view s, std::string_view prefix) noexcept -> bool {
            return s.size() >= prefix.size() && case_insensitive_equal{}(s.substr(0, prefix.size()), prefix);
        }
    }

    zip_archive::~zip_archive() {
        if (data_value_)
            munmap(const_cast<std::uint8_t *>(data_value_), size_);
    }

    auto zip_archive::open(const std::string &path) -> bool {
        path_ = path;

        int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
        if (fd < 0)
            return false;

        struct stat st{};
        if (fstat(fd, &st) != 0 || st.st_size <= 0) {
            close(fd);
            return false;
        }

        void *map = mmap(nullptr, static_cast<std::size_t>(st.st_size), PROT_READ, MAP_PRIVATE, fd, 0);
        close(fd);
        if (map == MAP_FAILED) {
            metin2::log::error("Cannot map archive {}: {}", path.c_str(), strerror(errno));
            return false;
        }

        data_value_ = static_cast<const std::uint8_t *>(map);
        size_ = static_cast<std::size_t>(st.st_size);

        if (!read_central_directory()) {
            metin2::log::error("Archive {} is not a valid zip archive", path.c_str());
            return false;
        }

        return true;
    }

    auto zip_archive::read_central_directory() -> bool {
        constexpr std::size_t eocd_size = 22;
        if (size_ < eocd_size)
            return false;

        // The end-of-central-directory record is followed by a comment of at most 64 KiB.
        std::size_t eocd = std::string::npos;
        std::size_t lowest = size_ > eocd_size + 0xFFFF ? size_ - eocd_size - 0xFFFF : 0;
        for (std::size_t pos = size_ - eocd_size + 1; pos-- > lowest;) {
            if (read32(data_value_ + pos) == kEndOfCentralDirSig) {
                eocd = pos;
                break;
            }
        }
        if (eocd == std::string::npos)
            return false;

        std::uint64_t count = read16(data_value_ + eocd + 10);
        std::uint64_t dir_size = read32(data_value_ + eocd + 12);
        std::uint64_t dir_offset = read32(data_value_ + eocd + 16);

        if ((count == 0xFFFF || dir_size == 0xFFFFFFFF || dir_offset == 0xFFFFFFFF) && eocd >= 20 &&
            read32(data_value_ + eocd - 20) == kZip64LocatorSig) {
            std::uint64_t zip64_end = read64(data_value_ + eocd - 20 + 8);
            if (zip64_end + 56 > size_ || read32(data_value_ + zip64_end) != kZip64EndSig)
                return false;
            count = read64(data_value_ + zip64_end + 32);
            dir_size = read64(data_value_ + zip64_end + 40);
            dir_offset = read64(data_value_ + zip64_end + 48);
        }

        if (dir_offset + dir_size > size_)
            return false;

        entries_.reserve(static_cast<std::size_t>(count));

        const std::uint8_t *p = data_value_ + dir_offset;
        const std::uint8_t *end = p + dir_size;
        for (std::uint64_t i = 0; i < count; ++i) {
            if (end - p < 46 || read32(p) != kCentralHeaderSig)
                return false;

            entry entry_value;
            entry_value.flags_value = read16(p + 8);
            entry_value.method = read16(p + 10);
            entry_value.crc = read32(p + 16);
            entry_value.compressed_size = read32(p + 20);
            entry_value.size = read32(p + 24);
            std::uint16_t name_length = read16(p + 28);
            std::uint16_t extra_length = read16(p + 30);
            std::uint16_t comment_length = read16(p + 32);
            entry_value.local_header = read32(p + 42);

            if (end - p < 46 + name_length + extra_length + comment_length)
                return false;

            // zip64 extended information: the 64-bit values for the fields saturated above, in order
            const std::uint8_t *extra = p + 46 + name_length;
            const std::uint8_t *extra_end = extra + extra_length;
            while (extra_end - extra >= 4) {
                std::uint16_t id = read16(extra);
                std::uint16_t length = read16(extra + 2);
                const std::uint8_t *field = extra + 4;
                if (extra_end - field < length)
                    break;
                if (id == 0x0001) {
                    const std::uint8_t *value = field;
                    auto next = [&](std::uint64_t &target) -> void {
                        if (target == 0xFFFFFFFF && field + length - value >= 8) {
                            target = read64(value);
                            value += 8;
                        }
                    };
                    next(entry_value.size);
                    next(entry_value.compressed_size);
                    next(entry_value.local_header);
                }
                extra = field + length;
            }

            std::string name(reinterpret_cast<const char *>(p + 46), name_length);
            if (!name.empty() && name.back() != '/')
                entries_.insert_or_assign(std::move(name), entry_value);

            p += 46 + name_length + extra_length + comment_length;
        }

        return true;
    }

    auto zip_archive::contains(std::string_view name) const -> bool {
        return entries_.find(std::string(name)) != entries_.end();
    }

    void zip_archive::for_each(std::string_view prefix, const std::function<void(const std::string &)> &fn) const {
        for (const auto &[name, entry] : entries_)
            if (assets::starts_with_no_case(name, prefix))
                fn(name);
    }

    auto zip_archive::get(mapped_file &mapped_file, std::string_view name, const void **data) const -> bool {
        auto it = entries_.find(std::string(name));
        if (it == entries_.end())
            return false;

        const entry &entry_value = it->second;
        if (entry_value.flags_value & kFlagEncrypted) {
            metin2::log::error("Archive {1}: {0} is encrypted (not supported)", it->first.c_str(), path_.c_str());
            return false;
        }

        if (entry_value.size > 0xFFFFFFFFull) {
            metin2::log::error("Archive {1}: {0} is larger than 4 GiB", it->first.c_str(), path_.c_str());
            return false;
        }

        if (entry_value.local_header + 30 > size_ || read32(data_value_ + entry_value.local_header) != kLocalHeaderSig) {
            metin2::log::error("Archive {1}: bad local header for {0}", it->first.c_str(), path_.c_str());
            return false;
        }

        std::uint64_t offset = entry_value.local_header + 30 + read16(data_value_ + entry_value.local_header + 26) + read16(data_value_ + entry_value.local_header + 28);
        if (offset + entry_value.compressed_size > size_) {
            metin2::log::error("Archive {1}: {0} runs past the end of the archive", it->first.c_str(), path_.c_str());
            return false;
        }

        const std::uint8_t *source = data_value_ + offset;

        switch (entry_value.method) {
        case kMethodStored:
            {
                if (entry_value.compressed_size != entry_value.size)
                    return false;
                mapped_file.link(static_cast<std::uint32_t>(entry_value.size), source);
                *data = source;
                return true;
            }

        case kMethodDeflate:
            {
                std::vector<std::uint8_t> buffer_value(static_cast<std::size_t>(entry_value.size));

                z_stream stream{};
                if (inflateInit2(&stream, -MAX_WBITS) != Z_OK)
                    return false;

                stream.next_in = const_cast<Bytef *>(source);
                stream.avail_in = static_cast<uInt>(entry_value.compressed_size);
                stream.next_out = buffer_value.data();
                stream.avail_out = static_cast<uInt>(buffer_value.size());

                int result = inflate(&stream, Z_FINISH);
                std::uint64_t produced = stream.total_out;
                inflateEnd(&stream);

                if (result != Z_STREAM_END || produced != entry_value.size) {
                    metin2::log::error("Archive {1}: cannot inflate {0} (zlib {2})", it->first.c_str(), path_.c_str(), result);
                    return false;
                }

                if (crc32(0, buffer_value.data(), static_cast<uInt>(buffer_value.size())) != entry_value.crc) {
                    metin2::log::error("Archive {1}: CRC mismatch for {0}", it->first.c_str(), path_.c_str());
                    return false;
                }

                *data = mapped_file.adopt(std::move(buffer_value));
                return true;
            }

        default:
            metin2::log::error("Archive {1}: {0} uses compression method {2} (only stored and deflate are supported)", it->first.c_str(), path_.c_str(), entry_value.method);
            return false;
        }
    }
}
