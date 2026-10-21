#pragma once

#include <cstddef>

namespace metin2::inline core {

    template <typename T>
    class func_object {
      public:
        func_object() {
            clear();
        }

        virtual ~func_object() = default;

        void clear() {
            self_object_ = nullptr;
            func_object_value_ = nullptr;
        }

        void set(T *self_object, void (T::*func_object)()) {
            self_object_ = self_object;
            func_object_value_ = func_object;
        }

        auto is_empty() -> bool {
            if (self_object_ != nullptr)
                return false;

            if (func_object_value_ != nullptr)
                return false;

            return true;
        }

        void run() {
            if (self_object_)
                if (func_object_value_)
                    (self_object_->*func_object_value_)();
        }

      protected:
        T *self_object_;
        void (T::*func_object_value_)();
    };
}
