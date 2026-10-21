#pragma once

#include <metin2/script/python_utils.h>

#include <cstdint>
#include <set>

namespace metin2::inline client::inline app {
    class python_exception_sender : public python_exception_sender_interface {
      public:
        python_exception_sender();

        ~python_exception_sender() override;

        void send() override;

      protected:
        std::set<uint32_t> set_sended_exception_crc_;
    };
}
