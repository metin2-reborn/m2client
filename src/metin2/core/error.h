#pragma once

namespace metin2::inline core {

    // Installs the crash handler (POSIX signals): appends a backtrace to crash.log in the log directory
    // (metin2::log::init must have run) and to stderr, then re-raises.
    void set_eter_exception_handler();
}
