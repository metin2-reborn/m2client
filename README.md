# Metin2 Client

[![CI](https://github.com/metin2-reborn/m2client/actions/workflows/ci.yml/badge.svg)](https://github.com/metin2-reborn/m2client/actions/workflows/ci.yml)
![C++23](https://img.shields.io/badge/C%2B%2B-23-00599C?logo=cplusplus)
![Platform](https://img.shields.io/badge/platform-Linux-FCC624?logo=linux&logoColor=black)
[![Last commit](https://img.shields.io/github/last-commit/metin2-reborn/m2client)](https://github.com/metin2-reborn/m2client/commits)
[![Issues](https://img.shields.io/github/issues/metin2-reborn/m2client)](https://github.com/metin2-reborn/m2client/issues)

The Metin2 game client, ported to Linux and cleaned up to the same standard as the server. It builds
with Clang or GCC and CMake, renders through its own renderer interface on OpenGL, plays sound through
OpenAL Soft and runs the Python 2.7 game scripts maintained in the `m2game` repository.

## Requirements

- Linux on x86-64
- Clang or GCC with C++23 support (developed with Clang 23 and GCC 16). Clang 19 cannot build
  OpenAL Soft; CI uses Clang 22.
- CMake 3.25+ and Ninja
- [just](https://github.com/casey/just) and [bubblewrap](https://github.com/containers/bubblewrap)
  (for the recipes and the sandbox below)
- Development packages for zlib, LZO 2, expat, fontconfig, OpenGL, X11 or Wayland, and ALSA,
  PulseAudio or PipeWire

On Debian or Ubuntu:

```sh
sudo apt install build-essential clang-22 cmake ninja-build pkgconf just bubblewrap \
    zlib1g-dev liblzo2-dev libexpat1-dev libfontconfig-dev libgl-dev \
    libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
    libwayland-dev wayland-protocols libxkbcommon-dev \
    libasound2-dev libpulse-dev libpipewire-0.3-dev
CC=clang-22 CXX=clang++-22 just build
```

raylib, OpenAL Soft, OpenGr2ndma (Granny), FreeType, minimp3, dr_libs, Monocypher, toml++, asio and
Python 2.7 are downloaded and built by CMake on the first configure.

## Building

```sh
just build                    # RelWithDebInfo build into build/
BUILD_TYPE=Debug just build   # Debug, with -Werror
CC=gcc CXX=g++ just build     # GCC instead of Clang
SANITIZE=1 just build         # AddressSanitizer + UndefinedBehaviorSanitizer into build-sanitize/
just test                     # build, then run the unit tests
just clean
```

The client ends up in `build/bin/client`.

## Running

The client needs the game data (`data/`, the bootstrap in `data/system`, the game scripts in
`data/game` and the `data/ui` package, `pack/`...). `just run` starts it from the game directory as
a normal process, with your own `~/.metin2`. The game directory defaults to an `m2game` checkout
next to this repository; set `METIN2_CLIENT_DIR` to use another one:

```sh
just run
METIN2_CLIENT_DIR=/path/to/client-data just run
```

`just run-sandbox` starts it in a bubblewrap sandbox instead: the game directory is mounted as an
overlay, so nothing can modify it, and `HOME` is `run/home` (a windowed
`run/home/.metin2/metin2.toml` is written on first use). The automation recipes below use the
sandbox. The server address is part of the game scripts' server list.

### ~/.metin2

| Path                       | Contents                                                          |
|----------------------------|-------------------------------------------------------------------|
| `metin2.toml`              | settings (display, graphics, sound, interface, login, log level)  |
| `interface.dat`            | window positions and UI state                                     |
| `log/client.log`           | log of the current run (the previous five are kept as `.1`-`.5`)  |
| `log/syserr.log`           | warnings and errors with file and line, Python tracebacks         |
| `log/crash.log`            | stack trace of a crash                                            |
| `mark/`                    | guild marks downloaded from the server                            |
| `screenshots/`             | screenshots                                                       |

A minimal `metin2.toml`:

```toml
[display]
width = 1280
height = 720
windowed = true
vsync = true
fps_limit = 0            # 0: no cap beyond vsync; e.g. 144
gamma = 3                # 1 (darker) to 5 (brighter), 3 is neutral

[graphics]
antialias = true         # FXAA on the 3D scene
shadow_level = 3
terrain_detail = 1       # relief shading from the terrain textures: 0 off, 1 low, 2 high
grass_density = 1        # grass on grass and field tiles: 0 off to 3

[interface]
font_scale = 1.0         # text size factor
font_antialias = true    # false gives the original 1-bit glyphs
ui_scale = 0             # interface magnification 1-3; 0 picks it from the window height
                         # (text is rasterized at the magnified size, not enlarged; on a scaled
                         # desktop the window is high-DPI and the automatic choice sees real pixels)
chat_timestamps = false  # prefix spoken chat lines with [HH:MM]

[keys]                   # optional: rebind game actions (names as in DIK_*, e.g. "F", "NUMPAD1", "GRAVE")
inventory = "I"
pick_up = "Z"

[sound]
music_volume = 0.5
voice_volume = 1.0

[log]
level = "info"           # "debug", "info", "warn" or "error"
```

Rebindable actions and their original keys: `attack` (SPACE), `move_up`/`move_down`/`move_left`/
`move_right` (arrows) and their `_alt` twins (W/S/A/D), `camera_left`/`camera_right` (Q/E),
`camera_zoom_in`/`camera_zoom_out` (R/F), `camera_pitch_up`/`camera_pitch_down` (T/G), `pick_up`
(Z) and `pick_up_alt` (`), `sprint` (LSHIFT, held), `character` (C), `skills` (V), `quests` (N),
`inventory` (I), `dragon_soul` (O), `map` (M), `chat_log` (L), `mount` (J), `help` (H), `emotions`
(B). Two actions bound to one key both fire.

A key with a wrong type or an unknown key is reported in `syserr.log` with its line and keeps its
default; the rest of the file still applies. `python_system::load_config` in
`src/metin2/client/app/python_system.cpp` reads every option.

### Development builds

Debug and RelWithDebInfo builds (`M2_DEV_TOOLS`) include an automation harness that drives
the client from a script or a socket: mouse and keyboard input, screenshots, Python execution and
frame stepping. Release and MinSizeRel builds contain none of it.

```sh
just smoke                       # start, reach the login screen, quit
just login                       # log-in screen checks
just world                       # load a map
just run-auto scripts/smoke.m2a  # any automation script
just send 'screenshot shot.png'  # one command to a running client (needs socat)
```

Scripts, commands and their results are logged to `run/automation.log`.

## Recipes

| Recipe            | Does                                                                |
|-------------------|---------------------------------------------------------------------|
| `build`           | configure if needed, then build                                     |
| `test`            | build, then run the unit tests                                      |
| `run`             | run the client from the game directory                              |
| `run-sandbox`     | run the client in the overlay sandbox with `HOME=run/home`          |
| `run-auto SCRIPT` | run in the sandbox with an automation script                        |
| `smoke`, `login`, `world` | the automation smoke tests                                  |
| `grn-dump`        | load every Granny file in the game data and summarize failures       |
| `checksum`, `verify-gamedir` | record and check a manifest of the game directory        |
| `format`          | format the sources with clang-format                                |
| `clean`           | remove the build and everything the client wrote under `run/`       |

## Contributing

Code style is defined by `.clang-format` and `.clang-tidy` in the repository root. Names follow the
standard library: `snake_case` for namespaces, types, functions and variables, a trailing `_` for private
and protected data members, `UPPER_CASE` for constants and enumerators. `clang-tidy` checks this.

- Format with `just format`. The files in `src/metin2/dev` keep their own style;
  `.clang-format-ignore` excludes them.
- Don't add trace or debug switches to the client. Tracing is added for one investigation and
  removed again; `.claude/skills/trace/SKILL.md` describes how.
- Keep headers self-contained, with `#pragma once`, and include what you use.
- Read game assets through `get_file_manager()` (`files/eter_file_manager.h`), never by path: asset
  names are case-insensitive and may live in the zip or the loose-file folder.
- The Python modules live under the root package `metin2` (`metin2.app`, `metin2.graphics.text`,
  `metin2.trade.shop`, ...); register a new one with `init_python_module` (`script/python_module.h`),
  which creates the parent packages. Export names (the strings in the module tables) are
  `snake_case`; the game scripts call them, so an export and its call sites in `m2game/data` change
  together.
- Wire structures in `client/protocol/packet.h` are packed and checked with `static_assert`; their
  sizes must match the server's.
- Comments explain what the code cannot: pitfalls, ordering, lifetimes, protocol facts.
- Build with both Clang and GCC, run `just test`, and run the smoke tests before sending a change.

## Third-party software

Licenses of bundled and fetched dependencies are in [`vendor/licenses`](vendor/licenses).

## Credits

Thanks to Mali for the Visual Studio 2022 client sources this port started from:
<https://metin2.dev/topic/22115-compile-client-with-visual-studio-2022-src/>
