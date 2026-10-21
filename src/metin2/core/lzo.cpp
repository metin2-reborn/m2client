#include "lzo.h"

#include <metin2/log/log.h>
#include "tea.h"
#include "utils.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace metin2::inline core {

    namespace {
        // Buffers below REUSING_CAPACITY are recycled instead of freed (pack reads are frequent).
        class lzo_free_memory_mgr {
          public:
            enum {
                REUSING_CAPACITY = 64 * 1024,
            };

            ~lzo_free_memory_mgr() {
                for (uint8_t *p : free_vector_)
                    delete[] p;

                free_vector_.clear();
            }

            auto alloc(unsigned capacity) -> uint8_t * {
                assert(capacity > 0);
                if (capacity < REUSING_CAPACITY) {
                    if (!free_vector_.empty()) {
                        uint8_t *free_mem = free_vector_.back();
                        free_vector_.pop_back();
                        return free_mem;
                    }
                    return new uint8_t[REUSING_CAPACITY];
                }
                return new uint8_t[capacity];
            }

            void free(uint8_t *ptr, unsigned capacity) {
                assert(ptr != nullptr);
                assert(capacity > 0);
                if (capacity < REUSING_CAPACITY) {
                    free_vector_.push_back(ptr);
                    return;
                }

                delete[] ptr;
            }

          private:
            std::vector<uint8_t *> free_vector_;
        };

        lzo_free_memory_mgr free_mem_mgr;

        // Scratch space for decryption: on the stack up to LOCAL_BUF_SIZE, otherwise on the heap.
        class decrypt_buffer {
          public:
            enum {
                LOCAL_BUF_SIZE = 8 * 1024,
            };

            explicit decrypt_buffer(unsigned size) {
                if (size >= LOCAL_BUF_SIZE)
                    buf_ = new char[size];
                else
                    buf_ = local_buf_;
            }

            ~decrypt_buffer() {
                if (local_buf_ != buf_)
                    delete[] buf_;
            }

            auto buffer_ptr() -> void * { return buf_; }

          private:
            char *buf_;
            char local_buf_[LOCAL_BUF_SIZE];
        };
    } // namespace

    uint32_t lz_object::four_cc = MAKEFOURCC('M', 'C', 'O', 'Z');

    lz_object::lz_object() {
        initialize();
    }

    void lz_object::initialize() {
        buffer_ = nullptr;
        buffer_size_ = 0;

        header_value_ = nullptr;
        in_ = nullptr;
        compressed_ = false;
    }

    void lz_object::clear() {
        if (buffer_)
            free_mem_mgr.free(buffer_, buffer_size_);

        initialize();
    }

    lz_object::~lz_object() {
        clear();
    }

    auto lz_object::size() -> uint32_t {
        assert(header_value_);

        if (compressed_) {
            if (header_value_->encrypt_size)
                return sizeof(header) + sizeof(uint32_t) + header_value_->encrypt_size;
            else
                return sizeof(header) + sizeof(uint32_t) + header_value_->compressed_size;
        } else
            return header_value_->real_size;
    }

    auto lz_object::compress() -> bool {
        lzo_uint out_len;
        uint8_t *buffer_value = buffer_ + sizeof(header);
        *reinterpret_cast<uint32_t *>(buffer_value) = four_cc;
        buffer_value += sizeof(uint32_t);

#if defined(LZO1X_999_MEM_COMPRESS)
        int r = lzo1x_999_compress(in_, header_value_->real_size, buffer_value, &out_len, get_clzo().work_memory());
#else
        int r = lzo1x_1_compress(m_pbIn, m_pHeader->dwRealSize, pbBuffer, &iOutLen, CLZO::Instance().GetWorkMemory());
#endif

        if (LZO_E_OK != r) {
            metin2::log::error("Cannot compress data with lzo1x_999");
            return false;
        }

        header_value_->compressed_size = static_cast<uint32_t>(out_len);
        compressed_ = true;
        return true;
    }

    auto lz_object::begin_decompress(const void *in) -> bool {
        auto *header_value = const_cast<header *>(static_cast<const header *>(in));

        if (header_value->four_cc != four_cc) {
            metin2::log::error("Data is not a valid LZO object");
            return false;
        }

        header_value_ = header_value;
        in_ = static_cast<const uint8_t *>(in) + (sizeof(header) + sizeof(uint32_t));

        buffer_size_ = header_value->real_size;
        buffer_ = free_mem_mgr.alloc(buffer_size_);
        memset(buffer_, 0, header_value->real_size);
        return true;
    }

    auto lz_object::decompress(uint32_t *key) -> bool {
        lzo_uint size;
        int r;

        if (header_value_->encrypt_size) {
            decrypt_buffer buf(header_value_->encrypt_size);

            auto *decrypted_buffer = static_cast<uint8_t *>(buf.buffer_ptr());

            decrypt_impl(key, decrypted_buffer);

            if (*reinterpret_cast<uint32_t *>(decrypted_buffer) != four_cc) {
                metin2::log::error("Cannot decrypt LZO object: wrong key");
                return false;
            }

            if (LZO_E_OK != (r = lzo1x_decompress(decrypted_buffer + sizeof(uint32_t), header_value_->compressed_size, buffer_, &size, nullptr))) {
                metin2::log::error("Cannot decompress decrypted LZO object (result {})", r);
                return false;
            }
        } else {
            size = header_value_->real_size;

            if (LZO_E_OK != (r = lzo1x_decompress(in_, header_value_->compressed_size, buffer_, &size, nullptr))) {
                metin2::log::error("Cannot decompress LZO object (result {}, compressed size {})", r, header_value_->compressed_size);
                return false;
            }
        }

        if (size != header_value_->real_size) {
            metin2::log::error("Decompressed size of LZO object does not match its header");
            return false;
        }

        return true;
    }

    auto lz_object::encrypt(uint32_t *key) -> bool {
        if (!compressed_) {
            assert(!"not compressed yet");
            return false;
        }

        uint8_t *buffer_value = buffer_ + sizeof(header);
        header_value_->encrypt_size = tea_encrypt(reinterpret_cast<uint32_t *>(buffer_value), reinterpret_cast<const uint32_t *>(buffer_value), key, header_value_->compressed_size + 19);
        return true;
    }

    auto lz_object::decrypt_impl(uint32_t *key, uint8_t *data) -> bool {
        assert(buffer_);

        tea_decrypt(reinterpret_cast<uint32_t *>(data), reinterpret_cast<const uint32_t *>(in_ - sizeof(uint32_t)), key, header_value_->encrypt_size);
        return true;
    }

    void lz_object::alloc_buffer(uint32_t src_size) {
        if (buffer_)
            free_mem_mgr.free(buffer_, buffer_size_);

        buffer_ = free_mem_mgr.alloc(src_size);
        buffer_size_ = src_size;
    }

    clzo::clzo() : work_mem_(nullptr) {
        if (lzo_init() != LZO_E_OK) {
            metin2::log::error("Cannot initialize LZO");
            return;
        }

#if defined(LZO1X_999_MEM_COMPRESS)
        work_mem_ = static_cast<uint8_t *>(malloc(LZO1X_999_MEM_COMPRESS));
#else
        m_pWorkMem = static_cast<uint8_t *>(malloc(LZO1X_1_MEM_COMPRESS));
#endif

        if (nullptr == work_mem_) {
            metin2::log::error("Cannot allocate LZO work memory");
            return;
        }
    }

    clzo::~clzo() {
        free(work_mem_);
        work_mem_ = nullptr;
    }

    auto clzo::decompress(lz_object &obj, const uint8_t *buf, uint32_t *key) -> bool {
        if (!obj.begin_decompress(buf))
            return false;

        return obj.decompress(key);
    }

    auto clzo::work_memory() -> uint8_t * {
        return work_mem_;
    }
}
