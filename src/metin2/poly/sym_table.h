#pragma once

#include <string>

namespace metin2::inline poly {

    class sym_table {
      public:
        sym_table(int tok, std::string str);
        virtual ~sym_table();

        double val;
        int token;
        std::string strlex;
    };
}
