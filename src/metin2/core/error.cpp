#include "error.h"
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <metin2/log/log.h>
#include <string>
#include <time.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
// After windows.h.
#include <dbghelp.h>
#else
#include <execinfo.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#endif

namespace metin2::inline core {

    // Built before any crash: the handler may only call async-signal-safe functions.
    static char crash_log_path[512];

#ifdef _WIN32

    // Unhandled exceptions (access violations, illegal instructions, ...): the stack goes to stderr
    // and crash.log with the symbols the PDB provides, then the exception continues to the system.
    static LONG WINAPI crash_filter(EXCEPTION_POINTERS *info) {
        static volatile LONG in_handler = 0;
        if (InterlockedExchange(&in_handler, 1))
            return EXCEPTION_CONTINUE_SEARCH;
        fflush(stderr);
        fflush(stdout);
        FILE *file = crash_log_path[0] ? fopen(crash_log_path, "a") : nullptr;
        const DWORD code = info && info->ExceptionRecord ? info->ExceptionRecord->ExceptionCode : 0;
        const void *address = info && info->ExceptionRecord ? info->ExceptionRecord->ExceptionAddress : nullptr;
        char buf[256];
        time_t now = time(nullptr);
        struct tm t;
        localtime_s(&t, &now);
        snprintf(buf, sizeof(buf), "\n==== metin2client crash: exception 0x%08lx at %04d-%02d-%02d %02d:%02d:%02d, address %p\n",
            static_cast<unsigned long>(code), t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, address);
        fputs(buf, stderr);
        if (file) fputs(buf, file);

        void *frames[64];
        const USHORT count = CaptureStackBackTrace(0, 64, frames, nullptr);
        HANDLE process = GetCurrentProcess();
        SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
        const bool symbols = SymInitialize(process, nullptr, TRUE) != FALSE;
        alignas(SYMBOL_INFO) char symbol_buffer[sizeof(SYMBOL_INFO) + 256];
        auto *symbol = reinterpret_cast<SYMBOL_INFO *>(symbol_buffer);
        for (USHORT i = 0; i < count; ++i) {
            memset(symbol_buffer, 0, sizeof(symbol_buffer));
            symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
            symbol->MaxNameLen = 255;
            DWORD64 displacement = 0;
            if (symbols && SymFromAddr(process, reinterpret_cast<DWORD64>(frames[i]), &displacement, symbol))
                snprintf(buf, sizeof(buf), "%s+0x%llx [%p]\n", symbol->Name, static_cast<unsigned long long>(displacement), frames[i]);
            else
                snprintf(buf, sizeof(buf), "[%p]\n", frames[i]);
            fputs(buf, stderr);
            if (file) fputs(buf, file);
        }
        if (symbols)
            SymCleanup(process);
        if (file)
            fclose(file);
        fflush(stderr);
        return EXCEPTION_CONTINUE_SEARCH;
    }

    void set_eter_exception_handler() {
        const std::string directory = metin2::log::directory();
        if (!directory.empty())
            snprintf(crash_log_path, sizeof(crash_log_path), "%s/crash.log", directory.c_str());

        SetUnhandledExceptionFilter(crash_filter);
    }

#else

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

#endif
}
