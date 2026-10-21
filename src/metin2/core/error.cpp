#include "error.h"
#include <cstdio>
#include <cstring>
#include <execinfo.h>
#include <fcntl.h>
#include <initializer_list>
#include <metin2/log/log.h>
#include <signal.h>
#include <string>
#include <time.h>
#include <unistd.h>

namespace metin2::inline core {

    // Built before any crash: the handler may only call async-signal-safe functions.
    static char crash_log_path[512];

    static void write_str(int fd, const char *s) {
        if (s) {
            size_t n = strlen(s);
            while (n) {
                ssize_t w = write(fd, s, n);
                if (w <= 0) break;
                s += w;
                n -= (size_t)w;
            }
        }
    }

    static void crash_handler(int sig, siginfo_t *info, void *) {
        static volatile sig_atomic_t in_handler = 0;
        if (in_handler) _exit(128 + sig);
        in_handler = 1;
        fflush(stderr);
        fflush(stdout);
        int fd = crash_log_path[0] ? open(crash_log_path, O_WRONLY | O_CREAT | O_APPEND, 0644) : -1;
        char buf[256];
        time_t now = time(nullptr);
        struct tm t;
        localtime_r(&now, &t);
        snprintf(buf, sizeof(buf), "\n==== metin2client crash: signal %d (%s) at %04d-%02d-%02d %02d:%02d:%02d, address %p\n",
            sig, strsignal(sig), t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, info ? info->si_addr : nullptr);
        write_str(2, buf);
        if (fd >= 0) write_str(fd, buf);
        void *frames[64];
        int n = backtrace(frames, 64);
        backtrace_symbols_fd(frames, n, 2);
        if (fd >= 0) {
            backtrace_symbols_fd(frames, n, fd);
            close(fd);
        }
        signal(sig, SIG_DFL);
        raise(sig);
    }

    void set_eter_exception_handler() {
        const std::string directory = metin2::log::directory();
        if (!directory.empty())
            snprintf(crash_log_path, sizeof(crash_log_path), "%s/crash.log", directory.c_str());

        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_sigaction = crash_handler;
        sa.sa_flags = SA_SIGINFO | SA_RESETHAND;
        sigemptyset(&sa.sa_mask);
        for (int sig : {SIGSEGV, SIGBUS, SIGFPE, SIGILL, SIGABRT})
            sigaction(sig, &sa, nullptr);
    }
}
