#include <clocale>

namespace metin2::inline client::inline app {
    auto run() -> int;
}

auto main() -> int {
    std::setlocale(LC_CTYPE, "C.UTF-8");
    
    return metin2::run();
}
