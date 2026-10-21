---
name: trace
description: Add temporary tracing to the Metin2 client to debug a problem (asset lookups, rendering, input, network, Python state), run it in the sandbox, read the output, then remove every trace line again. Use when a bug needs runtime evidence the logs do not already give.
---

# Temporary tracing in the client

The client has no built-in trace switches. Tracing is added for one investigation and removed
before the work is committed.

## Rules

- Mark every added line (or block) with `// TRACE-TEMP`, so it can be found and removed.
- Never commit tracing: `git grep -n TRACE-TEMP` must print nothing before a commit.
- Never send synthetic input to the user's desktop; reproduce in the sandbox (`just run-auto`).
- `just run-auto` rebuilds first: do not edit sources while a run is in progress.
- Do not write to the game data directory (`METIN2_CLIENT_DIR`, read-only); outputs go to `run/`,
  which is git-ignored.

## Writing trace output

- `metin2::log::error("TRACE ...")` (`<metin2/log/log.h>`) or `fprintf(stderr, ...)`: both end up in
  `run/home/.metin2/log/syserr.log` (the sandbox's `HOME` is `run/home`). Each run rotates the log;
  the previous five runs are `syserr.log.1` .. `.5`.
- For a lot of output (one line per file, packet or draw), open a file under `run/` once
  (`static FILE *f = fopen(".../run/trace.log", "a");`, line-buffered) and write there.
- Prefix lines with a short tag (`TRACE asset`, `TRACE gl`) so they can be filtered with grep.

## Reproducing

1. Copy an existing script from `run/*.m2a` (git-ignored; they hold the test account) or
   `scripts/*.m2a` to `run/<name>.m2a`. Login, character select and entering the game are the
   first ~20 lines of `run/walk_test.m2a`.
2. Script commands (PORTING.md, section 12): `wait`, `waitframes`, `click`, `key`, `keydown`,
   `keyup`, `text`, `wheel`, `resize <w> <h>` / `resize max`, `screenshot <file>`, `quit`, and
   `pyexec "<python>"`. Inside `pyexec`, `dbg.TraceError('...')` writes to syserr.log, so Python
   state (`player.GetMainCharacterPosition()`, `app.GetCamera()`, ...) can be traced without
   touching C++.
3. Run with `just run-auto run/<name>.m2a` (add `SANITIZE=1` for ASan/UBSan). Read
   `run/home/.metin2/log/syserr.log`, `run/automation.log` and the screenshots in `run/`.
4. For a before/after comparison, run the same script on the old code
   (`git stash`, run, `git stash pop`) and diff the outputs or screenshots.

## Hook points

| Question | Where to add the trace |
|---|---|
| Which asset is read, from where, or missing | `files/eter_file_manager.cpp`, `eter_file_manager::archive::get` (loose file, zip, or neither) |
| GL errors | `glGetError()` after the suspect call group in `gpu/opengl/device.cpp` / `resources.cpp`, or a `glDebugMessageCallback` installed in `gl_device::initialize` |
| Renderer conventions | flip the behaviour in `gpu/opengl/device.cpp` (the `half_pixel` flag passed to `pack_uniforms`, the cull winding in `apply_pipeline_state`, the 1x1 white texture for unbound stages) and compare screenshots |
| raylib / GLFW messages | `SetTraceLogLevel(LOG_INFO)` in `main_window::create` (`window/main_window.cpp`) |
| Mouse, pointer lock, warps | the event pump in `window/main_window.cpp` (`lock_pointer`, the warp handling) |
| Keyboard | `input/keymap.cpp` and the key events in `window/main_window.cpp` |
| Network packets | `net/net_stream.cpp` (send/receive) and the phase handlers in `client/protocol/` |
| Python calls into C++ | the module function in `client/**/python_*_module.cpp` or `ui/python_*_module.cpp` |
| libc-level file access | a temporary `-Wl,--wrap=open` (or `fopen`, `stat`) on the client link in `src/CMakeLists.txt` with `__wrap_`/`__real_` functions; heavy, use only when the asset manager is not the path |

## Removing it

1. `git diff` and `git grep -n TRACE-TEMP` list everything added.
2. Revert the files that only carry tracing (`git checkout -- <file>`), or delete the marked lines
   from files with real changes.
3. Rebuild (`just build`) and check that `git grep -n TRACE-TEMP` prints nothing.
4. Delete temporary scripts and logs in `run/` if they are no longer useful (they are ignored, but
   keep `run/` tidy).
