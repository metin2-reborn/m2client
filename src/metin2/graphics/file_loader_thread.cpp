#include "file_loader_thread.h"
#include <metin2/core/stl.h>
#include "resource_manager.h"

#include <cstring>

namespace metin2::inline graphics {

    file_loader_thread::file_loader_thread() = default;

    file_loader_thread::~file_loader_thread() {
        shutdown();
        destroy();
    }

    auto file_loader_thread::create() -> bool {
        if (thread_.joinable())
            return true;

        shutdowned_ = false;
        thread_ = std::thread([this] -> void { run(); });
        return true;
    }

    void file_loader_thread::destroy() {
        stl_wipe(request_deque_);
        stl_wipe(complete_deque_);
    }

    void file_loader_thread::shutdown() {
        if (!thread_.joinable())
            return;

        {
            std::lock_guard<std::mutex> lock(wake_mutex_);
            shutdowned_ = true;
        }
        wake_.notify_all();
        thread_.join();
    }

    void file_loader_thread::run() {
        while (!shutdowned_) {
            {
                std::unique_lock<std::mutex> lock(wake_mutex_);
                wake_.wait(lock, [this] -> bool { return pending_ > 0 || shutdowned_; });
                if (shutdowned_)
                    break;
                --pending_;
            }

            process();
        }
    }

    void file_loader_thread::request(std::string &file_name) // called in main thread
    {
        auto *data_value = new data;

        data_value->size = 0;
        data_value->buf = nullptr;
        data_value->file_name = file_name;

        request_mutex_.lock();
        request_deque_.push_back(data_value);
        request_mutex_.unlock();

        {
            std::lock_guard<std::mutex> lock(wake_mutex_);
            ++pending_;
        }
        wake_.notify_one();
    }

    auto file_loader_thread::fetch(data **data_value) -> bool // called in main thread
    {
        complete_mutex_.lock();

        if (complete_deque_.empty()) {
            complete_mutex_.unlock();
            return false;
        }

        *data_value = complete_deque_.front();
        complete_deque_.pop_front();

        complete_mutex_.unlock();
        return true;
    }

    void file_loader_thread::process() // called in loader thread
    {
        request_mutex_.lock();

        if (request_deque_.empty()) {
            request_mutex_.unlock();
            return;
        }

        data *data_value = request_deque_.front();
        request_deque_.pop_front();

        request_mutex_.unlock();

        const void *buf;

        if (resource_manager::file(data_value->file, data_value->file_name.c_str(), &buf)) {
            data_value->size = data_value->file.size();
            data_value->buf = new char[data_value->size];
            memcpy(data_value->buf, buf, data_value->size);
        }

        complete_mutex_.lock();
        complete_deque_.push_back(data_value);
        complete_mutex_.unlock();
    }
}
