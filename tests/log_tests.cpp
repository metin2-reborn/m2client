#include "check.h"

#include <metin2/log/log.h>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <thread>
#include <vector>

using namespace metin2;

namespace {
    namespace fs = std::filesystem;

    struct temp_log {
        fs::path directory;

        // One directory per test process: the runs of a test share it.
        static auto process_directory() -> fs::path {
            static const fs::path value = fs::temp_directory_path() / ("m2_client_log_test_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
            return value;
        }

        temp_log() : directory(process_directory()) {
            metin2::log::set_level(metin2::log::level::info);
            metin2::log::init(directory.string());
        }

        ~temp_log() { metin2::log::shutdown(); }

        [[nodiscard]] auto read(const std::string &name) const -> std::string {
            std::ifstream file(directory / name);
            return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
        }
    };

    auto count_lines(const std::string &text) -> std::size_t {
        std::size_t lines = 0;
        for (const char ch : text)
            lines += ch == '\n' ? 1 : 0;
        return lines;
    }

    enum class colour : std::uint8_t { red = 1,
        green = 2 };

    void remove_directory(const fs::path &directory) {
        std::error_code error;
        fs::remove_all(directory, error);
    }
}

TEST(log_writes_formatted_messages) {
    fs::path directory;
    {
        temp_log files;
        directory = files.directory;
        const char *missing = nullptr;
        int value = 7;
        metin2::log::info("player {} entered map {:d}", "Hero", 41);
        metin2::log::error("cannot open {} ({})", "item_proto", missing);
        metin2::log::info("colour {} at {}", colour::green, &value);

        const std::string all = files.read("client.log");
        const std::string errors = files.read("syserr.log");
        CHECK(all.find("[info] player Hero entered map 41\n") != std::string::npos);
        CHECK(all.find("cannot open item_proto ((null))") != std::string::npos);
        CHECK(all.find("colour 2 at 0x") != std::string::npos);
        CHECK(errors.find("[error]") != std::string::npos);
        CHECK(errors.find("log_tests.cpp:") != std::string::npos);
        CHECK(errors.find("player Hero") == std::string::npos);
    }
    remove_directory(directory);
}

TEST(log_filters_by_level) {
    fs::path directory;
    {
        temp_log files;
        directory = files.directory;
        metin2::log::debug("hidden");
        metin2::log::set_level(metin2::log::level::warn);
        metin2::log::info("also hidden");
        metin2::log::warn("shown");
        CHECK(!metin2::log::enabled(metin2::log::level::info));

        const std::string all = files.read("client.log");
        CHECK(all.find("hidden") == std::string::npos);
        CHECK(all.find("shown") != std::string::npos);
    }
    remove_directory(directory);
}

TEST(log_keeps_the_previous_runs) {
    fs::path directory;
    for (int run = 0; run < 7; ++run) {
        temp_log files;
        directory = files.directory;
        metin2::log::info("run {}", run);
    }
    std::ifstream latest(directory / "client.log");
    std::string line;
    std::getline(latest, line);
    CHECK(line.find("run 6") != std::string::npos);

    std::ifstream previous(directory / "client.log.1");
    std::getline(previous, line);
    CHECK(line.find("run 5") != std::string::npos);
    CHECK(fs::exists(directory / "client.log.5"));
    CHECK(!fs::exists(directory / "client.log.6"));
    remove_directory(directory);
}

TEST(log_is_safe_from_several_threads) {
    fs::path directory;
    {
        temp_log files;
        directory = files.directory;
        std::vector<std::thread> threads;
        for (int t = 0; t < 4; ++t)
            threads.emplace_back([t] {
                for (int i = 0; i < 250; ++i)
                    metin2::log::info("thread {} line {}", t, i);
            });
        for (std::thread &thread : threads)
            thread.join();

        CHECK_EQ(count_lines(files.read("client.log")), std::size_t{1000});
    }
    remove_directory(directory);
}
