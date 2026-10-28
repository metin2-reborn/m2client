# Building

## Requirements

- Linux on x86-64, or Windows (see below)
- Clang with C++23 support (developed with Clang 23; CI uses Clang 22). Clang 19 cannot build
  OpenAL Soft. Clang is the only supported compiler.
- CMake 3.25+ and Ninja
- [just](https://github.com/casey/just) and [bubblewrap](https://github.com/containers/bubblewrap)
  (for the recipes and the sandbox)
- Development packages for zlib, LZO 2, expat, OpenGL, X11 or Wayland, and ALSA,
  PulseAudio or PipeWire

On Debian or Ubuntu:

```sh
sudo apt install build-essential clang-22 cmake ninja-build pkgconf just bubblewrap \
    zlib1g-dev liblzo2-dev libexpat1-dev libgl-dev \
    libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
    libwayland-dev wayland-protocols libxkbcommon-dev \
    libasound2-dev libpulse-dev libpipewire-0.3-dev
CC=clang-22 CXX=clang++-22 just build
```

raylib, OpenAL Soft, OpenGr2ndma (Granny), FreeType, minimp3, dr_libs, Monocypher, toml++, asio and
Python 2.7 are downloaded and built by CMake on the first configure.

## Linux

```sh
just build                    # RelWithDebInfo build into build/
BUILD_TYPE=Debug just build   # Debug, with -Werror
CC=clang-22 CXX=clang++-22 just build   # a specific Clang (the default is `clang`)
SANITIZE=1 just build         # AddressSanitizer + UndefinedBehaviorSanitizer into build-sanitize/
just test                     # build, then run the unit tests
just clean
```

The client ends up in `build/bin/client`. `build/` is the tree `just run` starts, so keep it
RelWithDebInfo and put Debug builds in another directory, for example:

```sh
cmake -S . -B build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug && ctest --test-dir build-debug
```

## Windows

The same sources build on Windows with clang-cl, the MSVC-compatible Clang driver that ships with
Visual Studio (Desktop development with C++, with the "C++ Clang tools for Windows" component), CMake
and Ninja. From a Visual Studio developer prompt:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl -DCMAKE_LINKER=lld-link
cmake --build build
ctest --test-dir build
```

Nothing has to be installed besides the toolchain: zlib, LZO and Python 2.7.18 are built from
source, Python as a static library like on Linux, so `client.exe` has no DLLs beside it. The
settings, logs and screenshots live in `%APPDATA%\metin2`. The automation harness runs scripts on
Windows too, but its command socket is Linux only.

## Releases

The `Windows release` workflow (`.github/workflows/release-windows.yml`, started by hand with a
version `X.Y.Z` and an m2game ref) builds a Release `client.exe`, which uses the GUI subsystem so no
console window opens. It lays out the release folder with `scripts/package_release.py`:

| Path              | Contents                                                     |
|-------------------|--------------------------------------------------------------|
| `Metin2.exe`      | the client                                                   |
| `data/<pack>.zip` | one zip per folder of m2game's `data/`, paths relative to it |
| `lib/`            | m2game's `lib/` as it is (the Python standard library)       |

Files whose extension is in `EXCLUDE_EXTENSIONS` in the script (`.ase`) stay out of the packs. The
folder becomes `Metin2-<version>-windows-x64.zip` and an MSI built with WiX v5 from
`packaging/windows/metin2.wxs` (installs to Program Files, Start menu shortcut). Both are attached to
the release `v<version>` of m2game, tagged on the m2game commit that was packed. The workflow needs
the repository secret `M2GAME_TOKEN`: a token for `metin2-reborn/m2game` with Contents read and
write (m2game is private). The script runs on Linux too:
`scripts/package_release.py --exe build/bin/client --game ../m2game --out release`, then
`METIN2_CLIENT_DIR=$PWD/release just smoke` runs the client from the packs.

The data deflates to about 1.5 GB, against GitHub's 2 GiB limit per release asset and the 2 GB
limit of an MSI with an embedded cabinet; past that the packs have to be split or the cabinets
moved out of the MSI.
