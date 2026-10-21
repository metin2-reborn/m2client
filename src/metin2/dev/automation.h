#pragma once

#include <functional>
#include <string>

// The automation harness drives the client from a script or a socket (dev/automation.cpp). It exists
// in development builds only (M2_DEV_TOOLS); in release builds these are empty inline functions.
namespace metin2::inline dev {

    using py_exec_function = std::function<bool(const char *code, std::string *error)>;

#if M2_DEV_TOOLS
    // Reads the METIN2_AUTOMATION* environment and opens the script, socket and log.
    void init();
    // Runs the queued commands; called whenever the window collects its events.
    void tick();
    // Called before the frame is shown, with the finished back buffer (screenshots).
    void on_present();
    // Lets the pyexec command run Python inside the client.
    void set_py_exec(py_exec_function function);
#else
    inline void init() {}
    inline void tick() {}
    inline void on_present() {}
    inline void set_py_exec(py_exec_function) {}
#endif
}
