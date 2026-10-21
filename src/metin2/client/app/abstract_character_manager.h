#pragma once

#include <cstdint>

namespace metin2::inline client::inline characters {
    class instance_base;
}

namespace metin2::inline client::inline app {

    class abstract_character_manager {
      public:
        abstract_character_manager() = default;
        virtual ~abstract_character_manager() = default;

        virtual void destroy() = 0;
        virtual auto instance_ptr(uint32_t vid) -> instance_base * = 0;
    };

    auto get_abstract_character_manager() -> abstract_character_manager &;
}
