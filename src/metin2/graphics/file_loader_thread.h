#pragma once

#include <metin2/core/mapped_file.h>
#include <metin2/core/mutex.h>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <thread>

namespace metin2::inline graphics {

    // Background file loader: request() queues a file name, the thread reads it through the file
    // manager, fetch() hands the loaded buffers back on the main thread.
    class file_loader_thread {
      public:
        struct data {
            std::string file_name;

            mapped_file file;
            void *buf;
            uint32_t size;
        };

      public:
        file_loader_thread();
        ~file_loader_thread();

        auto create() -> bool;

      public:
        void request(std::string &file_name);
        auto fetch(data **data_value) -> bool;
        void shutdown();

      protected:
        void run();
        void destroy();
        void process();

      private:
        std::deque<data *> request_deque_;
        mutex request_mutex_;

        std::deque<data *> complete_deque_;
        mutex complete_mutex_;

        std::thread thread_;
        std::mutex wake_mutex_;
        std::condition_variable wake_;
        int pending_ = 0;
        std::atomic<bool> shutdowned_{false};
    };
}
