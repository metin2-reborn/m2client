#pragma once

#include <algorithm>
#include <map>
#include <string>
#include <vector>

namespace metin2::inline core {

    // ASCII-only lower case. Unlike std::tolower it does not depend on the locale and leaves the
    // bytes of UTF-8 sequences alone, which are negative as char.
    constexpr auto ascii_tolower(char c) noexcept -> char {
        return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
    }

    inline void stl_lowers(std::string &ret) {
        std::transform(ret.begin(), ret.end(), ret.begin(), ascii_tolower);
    }

    // Deletes every pointer of a container that owns its elements, then clears it.
    template <typename TContainer>
    inline void stl_wipe(TContainer &container) {
        for (auto &p : container)
            delete p;

        container.clear();
    }

    // Same for the mapped values of a map that owns them.
    template <typename TContainer>
    inline void stl_wipe_second(TContainer &container) {
        for (auto &kv : container)
            delete kv.second;

        container.clear();
    }

    using token_vector = std::vector<std::string>;
    using token_vector_map = std::map<std::string, token_vector>;
}
