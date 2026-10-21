#pragma once

#include <mutex>

namespace metin2::inline core {

    // Recursive lock with the Lock/Unlock interface the loader threads use.
    class mutex {
      public:
        void lock() { mutex_value_.lock(); }
        void unlock() { mutex_value_.unlock(); }

      private:
        std::recursive_mutex mutex_value_;
    };
}
