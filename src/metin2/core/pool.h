#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <metin2/log/log.h>
#include <vector>

namespace metin2::inline core {

    template <typename T>
    class dynamic_pool {
      public:
        dynamic_pool() {
            init_capacity_ = 0;
            used_capacity_ = 0;
        }
        virtual ~dynamic_pool() {
            assert(vct_data_.empty());
        }

        void set_name(const char *name) {
        }

        void clear() {
            destroy();
        }

        void destroy() {
            for (auto v : vct_data_)
                delete_value(v);
            vct_data_.clear();
            vct_free_.clear();
        }

        void create(uint32_t capacity) {
            init_capacity_ = capacity;
            vct_data_.reserve(capacity);
            vct_free_.reserve(capacity);
        }
        auto alloc() -> T * {
            if (vct_free_.empty()) {
                T *new_data = new T;
                vct_data_.push_back(new_data);
                ++used_capacity_;
                return new_data;
            }

            T *free_data = vct_free_.back();
            vct_free_.pop_back();
            return free_data;
        }
        void free(T *data) {
#ifdef DYNAMIC_POOL_STRICT
            assert(__IsValidData(pkData));
            assert(!__IsFreeData(pkData));
#endif
            vct_free_.push_back(data);
        }
        void free_all() {
            vct_free_ = vct_data_;
        }

        auto capacity() -> uint32_t {
            return vct_data_.size();
        }

      protected:
        auto is_valid_data(T *data) -> bool {
            if (vct_data_.end() == std::find(vct_data_.begin(), vct_data_.end(), data))
                return false;
            return true;
        }
        auto is_free_data(T *data) -> bool {
            if (vct_free_.end() == std::find(vct_free_.begin(), vct_free_.end(), data))
                return false;

            return true;
        }

        static void delete_value(T *data) {
            delete data;
        }

      protected:
        std::vector<T *> vct_data_;
        std::vector<T *> vct_free_;

        uint32_t init_capacity_;
        uint32_t used_capacity_;
    };

    template <typename T>
    class dynamic_pool_ex {
      public:
        dynamic_pool_ex() {
            init_capacity_ = 0;
            used_capacity_ = 0;
        }
        virtual ~dynamic_pool_ex() {
            assert(vct_free_.size() == vct_data_.size());
            destroy();
        }

        void clear() {
            destroy();
        }

        void destroy() {
            for (auto v : vct_data_)
                delete_value(v);
            vct_data_.clear();
            vct_free_.clear();
        }

        void create(uint32_t capacity) {
            init_capacity_ = capacity;
            vct_data_.reserve(capacity);
            vct_free_.reserve(capacity);
        }
        auto alloc() -> T * {
            if (vct_free_.empty()) {
                T *new_data = new_value();
                vct_data_.push_back(new_data);
                ++used_capacity_;
                return new_data;
            }

            T *free_data = vct_free_.back();
            vct_free_.pop_back();
            return free_data;
        }
        void free(T *data) {
#ifdef DYNAMIC_POOL_STRICT
            assert(__IsValidData(pkData));
            assert(!__IsFreeData(pkData));
#endif
            vct_free_.push_back(data);
        }
        void free_all() {
            vct_free_ = vct_data_;
        }

        auto capacity() -> uint32_t {
            return vct_data_.size();
        }

      protected:
        auto is_valid_data(T *data) -> bool {
            if (vct_data_.end() == std::find(vct_data_.begin(), vct_data_.end(), data))
                return false;
            return true;
        }
        auto is_free_data(T *data) -> bool {
            if (vct_free_.end() == std::find(vct_free_.begin(), vct_free_.end(), data))
                return false;

            return true;
        }

        static auto new_value() -> T * {
            return (T *)::operator new(sizeof(T));
        }
        static void delete_value(T *data) {
            ::operator delete(data);
        }

      protected:
        std::vector<T *> vct_data_;
        std::vector<T *> vct_free_;

        uint32_t init_capacity_;
        uint32_t used_capacity_;
    };

    template <class T>
    class pooled_object {
      public:
        pooled_object() = default;
        virtual ~pooled_object() = default;

        auto operator new(size_t /*mem_size*/) -> void * {
            return pool_value_.alloc();
        }

        void operator delete(void *t) {
            pool_value_.free((T *)t);
        }

        static void destroy_system() {
            pool_value_.Destroy();
        }

      protected:
        static dynamic_pool_ex<T> pool_value_;
    };

    template <class T>
    dynamic_pool_ex<T> pooled_object<T>::pool_value_;
}
