#include "sym_table.h"

namespace metin2::inline poly {

    sym_table::sym_table(int tok, std::string str) : val(0), token(tok), strlex(str) {
    }

    sym_table::~sym_table() = default;
}
