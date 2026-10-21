// Process entry point.
#include <clocale>

namespace metin2::inline client::inline app {
    auto run() -> int;
}

int main() {
    std::setlocale(LC_CTYPE, "C.UTF-8");
    return metin2::run();
}
