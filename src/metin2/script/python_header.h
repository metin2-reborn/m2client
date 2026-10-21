#pragma once

// The Python 2.7 headers predate C++17 (`register`) and pyconfig.h redefines feature macros that
// <features.h> and <unistd.h> already set; include them through this header so that the clean
// warning set does not reject them. The feature macros only matter before the first system header.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wregister"
#undef _POSIX_C_SOURCE
#undef _POSIX_SOURCE
#undef _XOPEN_SOURCE
#undef _POSIX_THREADS
#include <Python.h>

namespace metin2::inline script {
#pragma GCC diagnostic pop
}
