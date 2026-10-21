#include "file_base.h"

#include <algorithm>

namespace metin2::inline core {

    file_base::~file_base() {
        destroy();
    }

    auto file_base::file_name() -> const char * {
        return filename_.c_str();
    }

    void file_base::destroy() {
        close();
        size_ = 0;
    }

    void file_base::close() {
        if (fp_) {
            fclose(fp_);
            fp_ = nullptr;
        }
    }

    auto file_base::create(const char *filename, file_mode mode) -> bool {
        destroy();

        filename_ = filename;

        if (mode == FILEMODE_WRITE) {
            // OPEN_ALWAYS semantics: keep an existing file, create a missing one.
            fp_ = fopen(filename, "r+b");
            if (!fp_) {
                fp_ = fopen(filename, "w+b");
            }
        } else {
            fp_ = fopen(filename, "rb");
        }

        if (!fp_) {
            return false;
        }

        fseek(fp_, 0, SEEK_END);
        size_ = static_cast<uint32_t>(ftell(fp_));
        fseek(fp_, 0, SEEK_SET);

        mode_value_ = mode;

        return true;
    }

    auto file_base::size() -> uint32_t {
        return size_;
    }

    void file_base::seek(uint32_t offset) {
        if (offset > size_) {
            offset = size_;
        }

        fseek(fp_, offset, SEEK_SET);
    }

    auto file_base::get_position() -> uint32_t {
        return static_cast<uint32_t>(ftell(fp_));
    }

    auto file_base::write(const void *src, int bytes) -> bool {
        if (fwrite(src, 1, bytes, fp_) != static_cast<size_t>(bytes)) {
            return false;
        }

        fflush(fp_);
        size_ = std::max<uint32_t>(size_, static_cast<uint32_t>(ftell(fp_)));
        return true;
    }

    auto file_base::read(void *dest, int bytes) -> bool {
        // Like ReadFile, a short read at the end of the file is not an error.
        fread(dest, 1, bytes, fp_);
        return !ferror(fp_);
    }

    auto file_base::is_null() -> bool {
        return fp_ == nullptr;
    }
}
