#pragma once

namespace metin2::inline client::inline app {
    class abstract_chat {
      public:
        abstract_chat() = default;

        virtual ~abstract_chat() = default;

        virtual void append_chat(int type, const char *chat) = 0;
    };

    auto get_abstract_chat() -> abstract_chat &;
}
