// Automation harness: drives the client from a script file and/or a Unix domain socket.
//
// Enabled by METIN2_AUTOMATION=1 (or --automation on the command line). Commands (one per line):
//   wait <ms> | waitframes <n>
//   mousemove <x> <y> | mousedown <left|right|middle> | mouseup <btn> | click <btn> [x y] | dblclick <btn> [x y]
//   wheel <delta>
//   keydown <DIK_NAME|0xNN> | keyup <key> | key <key>          (held key + key event)
//   text "<utf-8 string>"                                      (one character event per code point)
//   screenshot <file.png>                                      (relative to METIN2_SCREENSHOT_DIR)
//   log "<text>" | pyexec "<python>" | ping | frame | audio | quit
// Every executed command and its result is appended to METIN2_AUTOMATION_LOG.
#if M2_DEV_TOOLS // development builds only (cmake/Flags.cmake)
#include "automation.h"
#include <metin2/input/input.h>
#include <metin2/input/keymap.h>
#include <metin2/sound/sound_manager.h>
#include <metin2/graphics/grp_base.h>
#include <metin2/graphics/image_file.h>
#include <metin2/window/main_window.h>
#ifndef _WIN32
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif
#include <cstring>
#include <deque>
#include <string>
#include <vector>
#include <functional>
#include <cstdarg>
#include <chrono>


namespace metin2::inline dev {

namespace {

struct Command {
    std::string line;
    int client = -1;  // socket fd to reply to, -1 for script
};

struct Pending {
    enum Kind { None, WaitMs, WaitFrames, Screenshot, Steps } kind = None;
    uint64_t until = 0;  // ms or frame number
    std::string path;
    Command cmd;
    // Steps: actions scheduled on later frames (mouse clicks need the game to see the new cursor
    // position in one frame, the button down in the next and the release after that).
    std::vector<std::pair<uint64_t, std::function<void()>>> steps;
};

bool g_enabled = false;
bool g_initialized = false;
std::string g_logPath, g_screenshotDir, g_socketPath;
FILE* g_log = nullptr;
std::deque<Command> g_queue;
Pending g_pending;
#ifndef _WIN32
int g_listenFd = -1;
std::vector<std::pair<int, std::string>> g_clients;  // fd, partial line buffer
#endif
py_exec_function g_pyExec;
bool g_quit = false;
std::vector<std::string> g_screenshotRequests;  // consumed in OnPresent

uint64_t NowMs() {
    return (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

void LogLine(const char* fmt, ...) {
    if (!g_log) return;
    char buf[2048];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    fprintf(g_log, "[%08llu %6llu] %s\n", (unsigned long long)NowMs(), (unsigned long long)window::frame_count(), buf);
    fflush(g_log);
}

void Reply(const Command& c, const std::string& text) {
    LogLine("%s -> %s", c.line.c_str(), text.c_str());
#ifndef _WIN32
    if (c.client >= 0) {
        std::string out = text + "\n";
        ssize_t r = send(c.client, out.data(), out.size(), MSG_NOSIGNAL);
        (void)r;
    }
#endif
}

std::vector<std::string> Tokenize(const std::string& line) {
    std::vector<std::string> out;
    size_t i = 0;
    while (i < line.size()) {
        while (i < line.size() && isspace((unsigned char)line[i])) ++i;
        if (i >= line.size()) break;
        std::string tok;
        if (line[i] == '"') {
            ++i;
            while (i < line.size() && line[i] != '"') {
                if (line[i] == '\\' && i + 1 < line.size()) { ++i; char e = line[i]; tok += e == 'n' ? '\n' : e == 't' ? '\t' : e; }
                else tok += line[i];
                ++i;
            }
            if (i < line.size()) ++i;
        } else {
            while (i < line.size() && !isspace((unsigned char)line[i])) tok += line[i++];
        }
        out.push_back(tok);
    }
    return out;
}

int ButtonIndex(const std::string& s) { if (s == "left" || s == "l") return 0; if (s == "right" || s == "r") return 1; if (s == "middle" || s == "m") return 2; return -1; }

void MouseButton(int btn, bool down) {
    static const window::mouse_button kButtons[] = { window::mouse_button::left, window::mouse_button::right, window::mouse_button::middle };
    window::push_event(window::mouse_button_event{ kButtons[btn], down, window::mouse_position() });
}

void KeyEvent(int dik, bool down) {
    input::set_key_override(dik, down);
    int vk = input::dik_to_vk(dik);
    if (vk) window::push_key(vk, down);
}

void TypeText(const std::string& utf8) {
    // decode UTF-8 to code points and queue a character event for each
    size_t i = 0;
    while (i < utf8.size()) {
        unsigned char c = (unsigned char)utf8[i];
        uint32_t cp;
        int len;
        if (c < 0x80) { cp = c; len = 1; }
        else if ((c & 0xE0) == 0xC0) { cp = c & 0x1F; len = 2; }
        else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F; len = 3; }
        else { cp = c & 0x07; len = 4; }
        for (int k = 1; k < len && i + k < utf8.size(); ++k) cp = (cp << 6) | ((unsigned char)utf8[i + k] & 0x3F);
        i += len;
        window::push_event(window::char_event{ (char32_t)cp });
    }
}

std::string ScreenshotPath(const std::string& name) {
    if (!name.empty() && name[0] == '/') return name;
    return g_screenshotDir + "/" + name;
}

// Returns true when the command completed synchronously (reply sent).
bool Execute(const Command& c) {
    std::vector<std::string> t = Tokenize(c.line);
    if (t.empty()) return true;
    const std::string& op = t[0];
    auto argi = [&](size_t i, int def = 0) { return i < t.size() ? atoi(t[i].c_str()) : def; };
    if (op == "wait" || op == "sleep") {
        g_pending = { Pending::WaitMs, NowMs() + (uint64_t)argi(1), "", c };
        return false;
    }
    if (op == "waitframes") {
        g_pending = { Pending::WaitFrames, window::frame_count() + (uint64_t)argi(1, 1), "", c };
        return false;
    }
    if (op == "mousemove") {
        window::set_mouse_override(window::point{ argi(1), argi(2) });
        Reply(c, "ok");
        return true;
    }
    if (op == "mouserelease") { window::set_mouse_override(std::nullopt); Reply(c, "ok"); return true; }
    if (op == "mousedown" || op == "mouseup" || op == "click" || op == "dblclick") {
        int btn = t.size() > 1 ? ButtonIndex(t[1]) : 0;
        if (btn < 0) { Reply(c, "err unknown button"); return true; }
        bool moved = false;
        if (t.size() > 3) { window::set_mouse_override(window::point{ argi(2), argi(3) }); moved = true; }
        uint64_t f = window::frame_count();
        g_pending = { Pending::Steps, 0, "", c };
        uint64_t at = f + (moved ? 1 : 0);  // let one frame see the new cursor position first
        if (op == "mousedown") g_pending.steps.push_back({ at, [btn] { MouseButton(btn, true); } });
        else if (op == "mouseup") g_pending.steps.push_back({ at, [btn] { MouseButton(btn, false); } });
        else {
            g_pending.steps.push_back({ at, [btn] { MouseButton(btn, true); } });
            g_pending.steps.push_back({ at + 1, [btn] { MouseButton(btn, false); } });
            if (op == "dblclick") {
                g_pending.steps.push_back({ at + 2, [btn] { MouseButton(btn, true); } });
                g_pending.steps.push_back({ at + 3, [btn] { MouseButton(btn, false); } });
            }
        }
        return false;
    }
    if (op == "wheel") {
        window::push_event(window::mouse_wheel_event{ argi(1, 120) });
        Reply(c, "ok");
        return true;
    }
    if (op == "keydown" || op == "keyup" || op == "key") {
        int dik = t.size() > 1 ? input::dik_from_name(t[1]) : 0;
        if (!dik) { Reply(c, "err unknown key"); return true; }
        if (op == "keydown") KeyEvent(dik, true);
        else if (op == "keyup") KeyEvent(dik, false);
        else {
            KeyEvent(dik, true);
            g_pending = { Pending::Steps, 0, "", c };
            g_pending.steps.push_back({ window::frame_count() + 2, [dik] { KeyEvent(dik, false); } });
            return false;
        }
        Reply(c, "ok");
        return true;
    }
    if (op == "text") { if (t.size() > 1) TypeText(t[1]); Reply(c, "ok"); return true; }
    if (op == "screenshot") {
        if (t.size() < 2) { Reply(c, "err missing path"); return true; }
        g_pending = { Pending::Screenshot, 0, ScreenshotPath(t[1]), c };
        g_screenshotRequests.push_back(g_pending.path);
        return false;
    }
    if (op == "resize") {
        if (t.size() > 1 && t[1] == "max") window::maximize_window();
        else if (t.size() > 2) window::resize_window(window::point{ argi(1), argi(2) });
        else { Reply(c, "err usage: resize <w> <h> | resize max"); return true; }
        Reply(c, "ok");
        return true;
    }
    if (op == "log") { Reply(c, t.size() > 1 ? t[1] : ""); return true; }
    if (op == "ping") { Reply(c, "pong"); return true; }
    if (op == "frame") { Reply(c, "frame " + std::to_string(window::frame_count())); return true; }
    if (op == "audio") { Reply(c, get_sound_manager().debug_status()); return true; }
    if (op == "pyexec") {
        if (!g_pyExec) { Reply(c, "err python not available"); return true; }
        std::string err;
        bool ok = g_pyExec(t.size() > 1 ? t[1].c_str() : "", &err);
        Reply(c, ok ? "ok" : "err " + err);
        return true;
    }
    if (op == "quit" || op == "exit") { g_quit = true; Reply(c, "ok"); window::request_quit(); return true; }
    Reply(c, "err unknown command");
    return true;
}

void LoadScript(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) { LogLine("cannot open script %s", path); return; }
    char line[4096];
    while (fgets(line, sizeof(line), f)) {
        std::string s = line;
        while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ')) s.pop_back();
        size_t p = s.find_first_not_of(" \t");
        if (p == std::string::npos || s[p] == '#') continue;
        g_queue.push_back({ s.substr(p), -1 });
    }
    fclose(f);
    LogLine("loaded script %s (%zu commands)", path, g_queue.size());
}

// The command socket is a Unix domain socket (socat on the developer's machine); on Windows only
// scripts drive the harness.
#ifdef _WIN32
void OpenSocket() {}
void PollSocket() {}
#else
void OpenSocket() {
    if (g_socketPath.empty()) return;
    unlink(g_socketPath.c_str());
    g_listenFd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (g_listenFd < 0) return;
    sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, g_socketPath.c_str(), sizeof(addr.sun_path) - 1);
    if (bind(g_listenFd, (sockaddr*)&addr, sizeof(addr)) != 0 || listen(g_listenFd, 4) != 0) {
        close(g_listenFd);
        g_listenFd = -1;
        LogLine("cannot listen on %s", g_socketPath.c_str());
        return;
    }
    LogLine("listening on %s", g_socketPath.c_str());
}

void PollSocket() {
    if (g_listenFd < 0) return;
    for (;;) {
        int fd = accept4(g_listenFd, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (fd < 0) break;
        g_clients.push_back({ fd, "" });
        LogLine("client connected");
    }
    for (size_t i = 0; i < g_clients.size();) {
        char buf[1024];
        ssize_t n = recv(g_clients[i].first, buf, sizeof(buf), 0);
        if (n == 0 || (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
            close(g_clients[i].first);
            g_clients.erase(g_clients.begin() + i);
            continue;
        }
        if (n > 0) {
            std::string& acc = g_clients[i].second;
            acc.append(buf, (size_t)n);
            size_t nl;
            while ((nl = acc.find('\n')) != std::string::npos) {
                std::string line = acc.substr(0, nl);
                acc.erase(0, nl + 1);
                while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
                if (!line.empty()) g_queue.push_back({ line, g_clients[i].first });
            }
        }
        ++i;
    }
}
#endif

}  // namespace

void init() {
    if (g_initialized) return;
    g_initialized = true;
    const char* env = getenv("METIN2_AUTOMATION");
    g_enabled = env && *env && strcmp(env, "0") != 0;
    const char* logPath = getenv("METIN2_AUTOMATION_LOG");
    const char* runDir = getenv("METIN2_RUN_DIR");
    std::string base = runDir && *runDir ? runDir : ".";
    g_logPath = logPath && *logPath ? logPath : base + "/automation.log";
    const char* shot = getenv("METIN2_SCREENSHOT_DIR");
    g_screenshotDir = shot && *shot ? shot : base;
    const char* sock = getenv("METIN2_AUTOMATION_SOCKET");
    g_socketPath = sock && *sock ? sock : base + "/automation.sock";
    if (!g_enabled) return;
    g_log = fopen(g_logPath.c_str(), "a");
    LogLine("automation enabled");
    const char* script = getenv("METIN2_AUTOMATION_SCRIPT");
    if (script && *script) LoadScript(script);
    OpenSocket();
}

void set_py_exec(py_exec_function fn) { g_pyExec = std::move(fn); }

void tick() {
    if (!g_enabled) return;
    PollSocket();
    // resolve pending waits
    if (g_pending.kind == Pending::WaitMs && NowMs() >= g_pending.until) { Reply(g_pending.cmd, "ok"); g_pending.kind = Pending::None; }
    else if (g_pending.kind == Pending::WaitFrames && window::frame_count() >= g_pending.until) {
        Reply(g_pending.cmd, "ok");
        g_pending.kind = Pending::None;
    } else if (g_pending.kind == Pending::Steps) {
        uint64_t f = window::frame_count();
        while (!g_pending.steps.empty() && g_pending.steps.front().first <= f) {
            g_pending.steps.front().second();
            g_pending.steps.erase(g_pending.steps.begin());
        }
        if (g_pending.steps.empty()) { Reply(g_pending.cmd, "ok"); g_pending.kind = Pending::None; }
    }
    // run queued commands until one blocks
    while (g_pending.kind == Pending::None && !g_queue.empty() && !g_quit) {
        Command c = g_queue.front();
        g_queue.pop_front();
        Execute(c);
    }
}

// The back buffer as an opaque PNG.
static bool CaptureBackBuffer(const char* path) {
    metin2::gpu::device& dev = metin2::graphic_base::device();
    std::vector<uint8_t> bgra;
    if (!dev.read_back_buffer(bgra)) return false;
    std::vector<uint32_t> pixels(bgra.size() / 4);
    memcpy(pixels.data(), bgra.data(), bgra.size());
    for (uint32_t& p : pixels) p |= 0xFF000000u;
    return metin2::save_png_file(path, (int)dev.back_buffer_width(), (int)dev.back_buffer_height(), pixels.data());
}

void on_present() {
    if (!g_enabled || g_screenshotRequests.empty()) return;
    for (const std::string& path : g_screenshotRequests) {
        bool ok = CaptureBackBuffer(path.c_str());
        if (g_pending.kind == Pending::Screenshot && g_pending.path == path) {
            Reply(g_pending.cmd, ok ? "ok " + path : "err screenshot failed");
            g_pending.kind = Pending::None;
        } else {
            LogLine("screenshot %s -> %s", path.c_str(), ok ? "ok" : "failed");
        }
    }
    g_screenshotRequests.clear();
}

}

#endif // M2_DEV_TOOLS
