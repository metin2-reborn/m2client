#pragma once

#include "reference_object.h"

#include <assert.h>
#include <cstddef>

namespace metin2::inline core {

    template <typename T>
    class ref_ptr {
      public:
        struct clear_fn {
            void operator()(ref_ptr<T> &ref) {
                ref.clear();
            }
        };

      public:
        ref_ptr() : object_(nullptr) {
        }

        ref_ptr(reference_object *object) {
            object_ = nullptr;
            initialize(object);
        }

        ref_ptr(const ref_ptr &ref) {
            object_ = nullptr;
            initialize(ref.object_);
        }

        ~ref_ptr() {
            clear();
        }

        void operator=(reference_object *object) {
            set_pointer(object);
        }

        void operator=(const ref_ptr &ref) {
            set_pointer(ref.object_);
        }

        void clear() {
            if (object_) {
                object_->release();
                object_ = nullptr;
            }
        }

        auto is_null() const -> bool {
            return object_ == nullptr ? true : false;
        }

        void set_pointer(reference_object *object) {
            reference_object *old_object = object_;

            object_ = object;

            if (object_)
                object_->add_reference();

            if (old_object)
                old_object->release();
        }

        auto pointer() const -> T * {
            return static_cast<T *>(object_);
        }

        auto operator->() const -> T * {
            assert(object_ != nullptr);
            return static_cast<T *>(object_);
        }

      private:
        void initialize(reference_object *object) {
            assert(object_ == nullptr);

            object_ = object;

            if (object_)
                object_->add_reference();
        }

      private:
        reference_object *object_;
    };
}
