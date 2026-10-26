#include <clocale>

namespace metin2::inline client::inline app {
    auto run() -> int;
}

auto main() -> int {
#ifdef _WIN32
    std::setlocale(LC_CTYPE, ".UTF-8");
#else
    std::setlocale(LC_CTYPE, "C.UTF-8");
#endif

    return metin2::run();
}
