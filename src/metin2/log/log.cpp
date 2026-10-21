#include "log.h"
#include <metin2/config/state_dir.h>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <mutex>

namespace metin2::log {
    namespace {
        constexpr int KEEP_RUNS = 5;

        struct state {
            std::mutex mutex_value;
            std::string directory;
            std::FILE *all = nullptr;
            bool errors_open = false;
        };

        auto shared() -> state & {
            static state instance;
            return instance;
        }

        std::atomic<level> threshold{level::info};

        constexpr std::array<std::string_view, 4> names{"debug", "info", "warn", "error"};

        auto name_of(level severity) -> std::string_view {
            return names[static_cast<std::size_t>(severity)];
        }

        auto timestamp() -> std::string {
            const auto now = std::chrono::floor<std::chrono::milliseconds>(std::chrono::current_zone()->to_local(std::chrono::system_clock::now()));
            return std::format("{:%Y-%m-%d %H:%M:%S}", now);
        }

        auto file_name(const char *path) -> std::string_view {
            const std::string_view text(path);
            const auto slash = text.rfind('/');
            return slash == std::string_view::npos ? text : text.substr(slash + 1);
        }

        // name -> name.1 -> ... -> name.KEEP_RUNS; the oldest is dropped.
        void keep_previous(const std::string &path) {
            std::remove(std::format("{}.{}", path, KEEP_RUNS).c_str());
            for (int run = KEEP_RUNS - 1; run >= 1; --run)
                std::rename(std::format("{}.{}", path, run).c_str(), std::format("{}.{}", path, run + 1).c_str());
            std::rename(path.c_str(), std::format("{}.1", path).c_str());
        }
    }

    auto init(const std::string &directory) -> bool {
        state &s = shared();
        const std::scoped_lock lock(s.mutex_value);

        ::metin2::state::make_directories(directory);
        s.directory = directory;
        const std::string all_path = directory + "/client.log";
        const std::string errors_path = directory + "/syserr.log";
        keep_previous(all_path);
        keep_previous(errors_path);

        s.all = std::fopen(all_path.c_str(), "w");
        s.errors_open = std::freopen(errors_path.c_str(), "w", stderr) != nullptr;
        if (!s.all || !s.errors_open) {
            std::fprintf(stderr, "cannot open the log files in %s\n", directory.c_str());
            return false;
        }
        return true;
    }

    auto directory() -> std::string {
        state &s = shared();
        const std::scoped_lock lock(s.mutex_value);
        return s.directory;
    }

    void shutdown() {
        state &s = shared();
        const std::scoped_lock lock(s.mutex_value);
        if (s.all)
            std::fclose(s.all);
        s.all = nullptr;
        std::fflush(stderr);
    }

    void set_level(level value) {
        threshold.store(value, std::memory_order_relaxed);
    }

    auto enabled(level severity) -> bool {
        return severity >= threshold.load(std::memory_order_relaxed);
    }

    void write(level severity, const std::source_location &where, std::string_view message) {
        const std::string stamp = timestamp();
        state &s = shared();
        const std::scoped_lock lock(s.mutex_value);

        if (severity >= level::warn) {
            const std::string line = std::format("{} [{}] {} ({}:{}): {}\n", stamp, name_of(severity), where.function_name(),
                file_name(where.file_name()), where.line(), message);
            std::fputs(line.c_str(), stderr);
            std::fflush(stderr);
            if (s.all) {
                std::fputs(line.c_str(), s.all);
                std::fflush(s.all);
            }
            return;
        }

        if (s.all) {
            const std::string line = std::format("{} [{}] {}\n", stamp, name_of(severity), message);
            std::fputs(line.c_str(), s.all);
            std::fflush(s.all);
        }
#if M2_DEV_TOOLS
        if (threshold.load(std::memory_order_relaxed) == level::debug)
            std::printf("[%.*s] %.*s\n", static_cast<int>(name_of(severity).size()), name_of(severity).data(), static_cast<int>(message.size()), message.data());
#endif
    }
}
