#pragma once

namespace metin2::inline world::inline actors {

    // Lets actors query map blocking, so Gungsin Tanyoung cannot leave the character stuck in the map.
    class background_interface {
      public:
        background_interface() = default;
        virtual ~background_interface() = default;

        virtual auto is_block(int x, int y) -> bool = 0;
    };

    auto get_background_interface() -> background_interface &;
}
